#include "PluginEditor.h"

// ==============================================
// UI构造：初始化所有控件 + 绑定参数
// ==============================================
BandSplitDistortAudioProcessorEditor::BandSplitDistortAudioProcessorEditor(
    BandSplitDistortAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    // 统一旋钮样式：圆形旋钮 + 下方数值框
    initRotarySlider(sliderFreqLow);
    initRotarySlider(sliderFreqHigh);
    initRotarySlider(sliderDriveLow);
    initRotarySlider(sliderDriveMid);
    initRotarySlider(sliderDriveHigh);
    initRotarySlider(sliderSendMix);

    // ---------- 按钮文字标注 ----------
    btnColorEnable.setButtonText("启用跨频段染色");

    btnSendLow.setButtonText("发送 - 低频");
    btnSendMid.setButtonText("发送 - 中频");
    btnSendHigh.setButtonText("发送 - 高频");

    btnRecvLow.setButtonText("接收 - 低频");
    btnRecvMid.setButtonText("接收 - 中频");
    btnRecvHigh.setButtonText("接收 - 高频");

    // ==============================================
    // 所有参数绑定（与Processor参数ID严格对应）
    // ==============================================
    attFreqLow      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "Freq_Low", sliderFreqLow);
    attFreqHigh     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "Freq_High", sliderFreqHigh);

    attDriveLow     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "Drive_Low", sliderDriveLow);
    attDriveMid     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "Drive_Mid", sliderDriveMid);
    attDriveHigh    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "Drive_High", sliderDriveHigh);

    attColorEnable  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "Color_Enable", btnColorEnable);

    attSendLow      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "Send_Low", btnSendLow);
    attSendMid      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "Send_Mid", btnSendMid);
    attSendHigh     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "Send_High", btnSendHigh);

    attRecvLow      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "Recv_Low", btnRecvLow);
    attRecvMid      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "Recv_Mid", btnRecvMid);
    attRecvHigh     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "Recv_High", btnRecvHigh);

    attSendMix      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "Send_Mix", sliderSendMix);

    // 全部加入可视界面
    addAndMakeVisible(sliderFreqLow);
    addAndMakeVisible(sliderFreqHigh);
    addAndMakeVisible(sliderDriveLow);
    addAndMakeVisible(sliderDriveMid);
    addAndMakeVisible(sliderDriveHigh);
    addAndMakeVisible(sliderSendMix);

    addAndMakeVisible(btnColorEnable);
    addAndMakeVisible(btnSendLow);
    addAndMakeVisible(btnSendMid);
    addAndMakeVisible(btnSendHigh);
    addAndMakeVisible(btnRecvLow);
    addAndMakeVisible(btnRecvMid);
    addAndMakeVisible(btnRecvHigh);

    // 窗口固定尺寸
    setSize(780, 340);
}

// ==============================================
// 析构
// ==============================================
BandSplitDistortAudioProcessorEditor::~BandSplitDistortAudioProcessorEditor()
{
}

// ==============================================
// 旋钮样式初始化函数
// ==============================================
void BandSplitDistortAudioProcessorEditor::initRotarySlider(juce::Slider& s)
{
    // 圆形拖拽旋钮
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    // 下方文本框、固定大小
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
}

// ==============================================
// 背景绘制 + 标题说明
// ==============================================
void BandSplitDistortAudioProcessorEditor::paint(juce::Graphics& g)
{
    // 深色背景
    g.fillAll(juce::Colour(22, 26, 32));
    g.setColour(juce::Colours::lightcyan);
    g.setFont(17.0f);

    // 主标题
    g.drawText("Band-Split Segmented Distortion | 频段分段失真 VST2/VST3",
        0, 8, getWidth(), 24, juce::Justification::centred);

    // 分区小字标注
    g.setFont(13.0f);
    g.setColour(juce::Colours::grey);
    g.drawText("分频点", 30, 35, 120, 20, juce::Justification::left);
    g.drawText("各频段失真度", 220, 35, 180, 20, juce::Justification::left);
    g.drawText("染色路由控制", 480, 35, 200, 20, juce::Justification::left);
}

// ==============================================
// 控件排版布局
// ==============================================
void BandSplitDistortAudioProcessorEditor::resized()
{
    // 基础尺寸
    const int knobW = 90;
    const int knobH = 105;

    // ---------- 左侧：分频点 ----------
    sliderFreqLow.setBounds(20, 55, knobW, knobH);
    sliderFreqHigh.setBounds(115, 55, knobW, knobH);

    // ---------- 中间：三段独立失真 ---------
    sliderDriveLow.setBounds(220, 55, knobW, knobH);
    sliderDriveMid.setBounds(315, 55, knobW, knobH);
    sliderDriveHigh.setBounds(410, 55, knobW, knobH);

    // ---------- 右侧：染色混合比例 ----------
    sliderSendMix.setBounds(520, 55, knobW, knobH);

    // ---------- 下方：染色 & 发送/接收开关组 ----------
    btnColorEnable.setBounds(20, 180, 160, 28);

    // 发送频段
    btnSendLow.setBounds(20, 215, 120, 24);
    btnSendMid.setBounds(150, 215, 120, 24);
    btnSendHigh.setBounds(280, 215, 120, 24);

    // 接收频段
    btnRecvLow.setBounds(410, 215, 120, 24);
    btnRecvMid.setBounds(540, 215, 120, 24);
    btnRecvHigh.setBounds(670, 215, 120, 24);
}