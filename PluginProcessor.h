#pragma once
#include <JuceHeader.h>

// ==============================
// 分频式三段分段失真 + 跨频段染色系统
// 核心：频段拆分 ≠ 音量拆分
// ==============================
class BandSplitDistortAudioProcessor : public juce::AudioProcessor
{
public:
    BandSplitDistortAudioProcessor();
    ~BandSplitDistortAudioProcessor() override;

    void prepareToPlay(double sr, int blockSize) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // 主音频处理
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // UI
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // 基础信息
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // 预设 & 存档
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int idx) override;
    const juce::String getProgramName(int idx) override;
    void changeProgramName(int idx, const juce::String& name) override;

    void getStateInformation(juce::MemoryBlock& data) override;
    void setStateInformation(const void* data, int size) override;

    // 参数树
    juce::AudioProcessorValueTreeState apvts;

private:
    // ========== 1. 分频滤波器 【三段拆分核心】 ==========
    juce::IIRFilter lowFilterL,  lowFilterR;
    juce::IIRFilter midFilterL,  midFilterR;
    juce::IIRFilter highFilterL, highFilterR;

    float freqLow  = 200.0f;
    float freqHigh = 3000.0f;

    // ========== 2. 单段失真塑形 ==========
    float distortShape(float in, float drive, bool colorMode);

    // ========== 3. 染色合并处理 ==========
    juce::AudioBuffer<float>  bandMergeBuffer;  // 多频段合并临时缓冲

    // 更新滤波器系数
    void updateFilters(double sampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandSplitDistortAudioProcessor)
};