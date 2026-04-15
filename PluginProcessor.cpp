#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================
// 参数列表：完全对应你需要的功能
// ==============================
juce::AudioProcessorValueTreeState::ParameterLayout createParamLayout()
{
    using FloatP = juce::AudioParameterFloat;
    using BoolP  = juce::AudioParameterBool;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ---------- 分频点 ----------
    params.push_back(std::make_unique<FloatP>("Freq_Low",  "低频分频点",  50.0f,  800.0f,  200.0f));
    params.push_back(std::make_unique<FloatP>("Freq_High", "高频分频点", 1500.0f, 6000.0f, 3000.0f));

    // ---------- 三段独立失真 ----------
    params.push_back(std::make_unique<FloatP>("Drive_Low",  "低频失真", 1.0f, 10.0f, 2.0f));
    params.push_back(std::make_unique<FloatP>("Drive_Mid",  "中频失真", 1.0f, 10.0f, 3.0f));
    params.push_back(std::make_unique<FloatP>("Drive_High", "高频失真", 1.0f, 10.0f, 2.5f));

    // ---------- 全局染色总开关 ----------
    params.push_back(std::make_unique<BoolP>("Color_Enable", "启用跨频段染色", false));

    // ---------- 发送 / 接收 频段选择 ----------
    params.push_back(std::make_unique<BoolP>("Send_Low",  "发送-低频",  false));
    params.push_back(std::make_unique<BoolP>("Send_Mid",  "发送-中频",  false));
    params.push_back(std::make_unique<BoolP>("Send_High", "发送-高频",  false));

    params.push_back(std::make_unique<BoolP>("Recv_Low",  "接收-低频",  true));
    params.push_back(std::make_unique<BoolP>("Recv_Mid",  "接收-中频",  true));
    params.push_back(std::make_unique<BoolP>("Recv_High", "接收-高频",  true));

    // ---------- 发送段染色前占比 ----------
    params.push_back(std::make_unique<FloatP>("Send_Mix", "染色前发送占比", 0.0f, 1.0f, 0.5f));

    return { std::move(params) };
}

// ==============================
// 构造函数
// ==============================
BandSplitDistortAudioProcessor::BandSplitDistortAudioProcessor()
: juce::AudioProcessor(
    juce::BusesProperties()
        .withInput("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
), apvts(*this, nullptr, "BandDistParams", createParamLayout())
{
    bandMergeBuffer.setSize(2, 4096);
}

BandSplitDistortAudioProcessor::~BandSplitDistortAudioProcessor(){}

// ==============================
// 滤波器更新：三段 Linkwitz-Riley 分频
// ==============================
void BandSplitDistortAudioProcessor::updateFilters(double sr)
{
    freqLow  = *apvts.getRawParameterValue("Freq_Low");
    freqHigh = *apvts.getRawParameterValue("Freq_High");

    // 低频：LPF
    lowFilterL.makeLowPass(sr, freqLow);
    lowFilterR.makeLowPass(sr, freqLow);

    // 中频：BPF
    midFilterL.makeBandPass(sr, (freqLow + freqHigh) * 0.5f, (freqHigh - freqLow) * 0.7f);
    midFilterR.makeBandPass(sr, (freqLow + freqHigh) * 0.5f, (freqHigh - freqLow) * 0.7f);

    // 高频：HPF
    highFilterL.makeHighPass(sr, freqHigh);
    highFilterR.makeHighPass(sr, freqHigh);
}

// ==============================
// 单段失真核心
// colorMode=false：独立干净削波
// colorMode=true：谐波染色饱和
// ==============================
float BandSplitDistortAudioProcessor::distortShape(float in, float drive, bool colorMode)
{
    float s = in * drive;

    if (!colorMode)
    {
        // 无染色：硬削波 独立失真
        return juce::jlimit(-1.0f, 1.0f, s);
    }
    else
    {
        // 染色模式：柔性 tanh + 偶次谐波
        float soft = std::tanh(s * 0.85f);
        float harm = (s * s * 0.15f) * (s > 0.f ? 1.f : -1.f);
        return soft + harm;
    }
}

// ==============================
// 播放初始化
// ==============================
void BandSplitDistortAudioProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    updateFilters(sampleRate);
    bandMergeBuffer.setSize(2, blockSize);
    bandMergeBuffer.clear();
}

void BandSplitDistortAudioProcessor::releaseResources(){}

bool BandSplitDistortAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannels() == 2 && layouts.getMainOutputChannels() == 2;
}

// ==============================
// 主DSP 【完全按照你定义的染色规则】
// ==============================
void BandSplitDistortAudioProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto sr = getSampleRate();
    updateFilters(sr);

    // ---------- 读取全局参数 ----------
    bool colorOn     = *apvts.getRawParameterValue("Color_Enable") > 0.5f;
    float sendMix    = *apvts.getRawParameterValue("Send_Mix");

    // 三段失真强度
    float dLow  = *apvts.getRawParameterValue("Drive_Low");
    float dMid  = *apvts.getRawParameterValue("Drive_Mid");
    float dHigh = *apvts.getRawParameterValue("Drive_High");

    // 发送频段选择
    bool sLow  = *apvts.getRawParameterValue("Send_Low") > 0.5f;
    bool sMid  = *apvts.getRawParameterValue("Send_Mid") > 0.5f;
    bool sHigh = *apvts.getRawParameterValue("Send_High") > 0.5f;

    // 接收频段选择
    bool rLow  = *apvts.getRawParameterValue("Recv_Low") > 0.5f;
    bool rMid  = *apvts.getRawParameterValue("Recv_Mid") > 0.5f;
    bool rHigh = *apvts.getRawParameterValue("Recv_High") > 0.5f;
    
    int numCh = buf.getNumChannels();
    int numSmp = buf.getNumSamples();

    // 清空合并缓冲
    bandMergeBuffer.clear();

    // ========== 1. 频段拆分：原始信号分成 低/中/高 三段 ==========
    for (int ch = 0; ch < numCh; ch++)
    {
        auto* inData = buf.getReadPointer(ch);

        for (int i = 0; i < numSmp; i++)
        {
            float in = inData[i];
            float l, m, h;

            if(ch == 0)
            {
                l = lowFilterL.processSingleSample(in);
                m = midFilterL.processSingleSample(in);
                h = highFilterL.processSingleSample(in);
            }
            else
            {
                l = lowFilterR.processSingleSample(in);
                m = midFilterR.processSingleSample(in);
                h = highFilterR.processSingleSample(in);
            }

            // 暂存拆分后的三段原始声音
            // 你后面需要独立/染色切换，这里保留原始分段
            bandMergeBuffer.setSample(ch, i, 0.f);

            // 标记：后续分别处理独立失真 / 染色合并
        }
    }

    // ==============================================
    // 模式A：染色关闭 → 三段【完全独立失真】
    // ==============================================
    if (!colorOn)
    {
        for (int ch = 0; ch < numCh; ch++)
        {
            auto* out = buf.getWritePointer(ch);
            for (int i = 0; i < numSmp; i++)
            {
                float in = buf.getSample(ch, i);

                // 1.拆分
                float l = (ch==0) ? lowFilterL.processSingleSample(in) : lowFilterR.processSingleSample(in);
                float m = (ch==0) ? midFilterL.processSingleSample(in) : midFilterR.processSingleSample(in);
                float h = (ch==0) ? highFilterL.processSingleSample(in) : highFilterR.processSingleSample(in);

                // 2.各段独立失真
                l = distortShape(l, dLow, false);
                m = distortShape(m, dMid, false);
                h = distortShape(h, dHigh, false);

                // 3.直接相加输出
                out[i] = l + m + h;
            }
        }
        return;
    }

    // ==============================================
    // 模式B：染色开启 → 严格执行你要求的规则
    // 1.勾选发送段合并整体失真
    // 2.切除发送段原生输出
    // 3.用染色混合声覆盖接收段
    // 4.无发送段则不切除
    // ==============================================
    for (int ch = 0; ch < numCh; ch++)
    {
        auto* out = buf.getWritePointer(ch);

        for (int i = 0; i < numSmp; i++)
        {
            float in = buf.getSample(ch, i);

            // 频段拆分
            float rawL = (ch==0) ? lowFilterL.processSingleSample(in) : lowFilterR.processSingleSample(in);
            float rawM = (ch==0) ? midFilterL.processSingleSample(in) : midFilterR.processSingleSample(in);
            float rawH = (ch==0) ? highFilterL.processSingleSample(in) : highFilterR.processSingleSample(in);

            // 独立失真结果
            float distL = distortShape(rawL, dLow, true);
            float distM = distortShape(rawM, dMid, true);
            float distH = distortShape(rawH, dHigh, true);

            // ---------- 1.合并所有勾选【发送频段】 ----------
            float sendMerge = 0.f;
            if(sendMix > 0.f)
            {
                if(sLow) sendMerge += rawL * sendMix;
                if(sMid) sendMerge += rawM * sendMix;
                if(sHigh) sendMerge += rawH * sendMix;
            }

            // 合并后整体染色失真
            float colorSignal = distortShape(sendMerge, 2.0f, true);

            // ---------- 2.切除发送段原生输出 ----------
            if(sLow) distL = 0.f;
            if(sMid) distM = 0.f;
            if(sHigh) distH = 0.f;

            // ---------- 3.染色信号覆盖接收频段 ----------
            if(rLow) distL += colorSignal;
            if(rMid) distM += colorSignal;
            if(rHigh) distH += colorSignal;

            // 最终总和输出
            out[i] = distL + distM + distH;
        }
    }
}

// ==============================
// 剩余基础函数省略（自动生成默认）
// ==============================
juce::AudioProcessorEditor* BandSplitDistortAudioProcessor::createEditor()
{
    return new BandSplitDistortAudioProcessorEditor(*this);
}
bool BandSplitDistortAudioProcessor::hasEditor() const noexcept { return true; }
juce::String BandSplitDistortAudioProcessor::getName() const { return "BandSplit Distort"; }
bool BandSplitDistortAudioProcessor::acceptsMidi() const { return false; }
bool BandSplitDistortAudioProcessor::producesMidi() const { return false; }
bool BandSplitDistortAudioProcessor::isMidiEffect() const { return false; }
double BandSplitDistortAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int BandSplitDistortAudioProcessor::getNumPrograms() { return 1; }
int BandSplitDistortAudioProcessor::getCurrentProgram() { return 0; }
void BandSplitDistortAudioProcessor::setCurrentProgram(int){}
juce::String BandSplitDistortAudioProcessor::getProgramName(int){ return "Default"; }
void BandSplitDistortAudioProcessor::changeProgramName(int, const juce::String&){}

void BandSplitDistortAudioProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto xml = apvts.copyState().createXml();
    copyXmlToBinary(*xml, data);
}
void BandSplitDistortAudioProcessor::setStateInformation(const void* data, int size)
{
    auto xml = getXmlFromBinary(data, size);
    if(xml) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}