#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ==============================================
// 分频分段失真 UI面板
// 功能：分频调节 / 三段独立失真 / 跨频段染色路由
// ==============================================
class BandSplitDistortAudioProcessorEditor 
    : public juce::AudioProcessorEditor
{
public:
    // 构造 & 析构
    BandSplitDistortAudioProcessorEditor(BandSplitDistortAudioProcessor&);
    ~BandSplitDistortAudioProcessorEditor() override;

    // 绘制背景、文字
    void paint(juce::Graphics& g) override;
    // 控件自动布局
    void resized() override;

private:
    // 处理器引用
    BandSplitDistortAudioProcessor& proc;

    // ========== 1. 分频调节旋钮 ==========
    juce::Slider sliderFreqLow;
    juce::Slider sliderFreqHigh;

    // ========== 2. 三段独立失真旋钮 ==========
    juce::Slider sliderDriveLow;
    juce::Slider sliderDriveMid;
    juce::Slider sliderDriveHigh;

    // ========== 3. 染色全局开关 ==========
    juce::ToggleButton btnColorEnable;

    // ========== 4. 发送频段选择（Low/Mid/High） ==========
    juce::ToggleButton btnSendLow;
    juce::ToggleButton btnSendMid;
    juce::ToggleButton btnSendHigh;

    // ========== 5. 接收频段选择（Low/Mid/High） ==========
    juce::ToggleButton btnRecvLow;
    juce::ToggleButton btnRecvMid;
    juce::ToggleButton btnRecvHigh;

    // ========== 6. 染色前 发送段占比 ==========
    juce::Slider sliderSendMix;

    // ========== 参数绑定器（UI <-> 音频参数） ==========
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attFreqLow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attFreqHigh;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attDriveLow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attDriveMid;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attDriveHigh;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attColorEnable;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attSendLow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attSendMid;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attSendHigh;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attRecvLow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attRecvMid;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attRecvHigh;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attSendMix;

    // 快速初始化旋钮样式
    void initRotarySlider(juce::Slider& s);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandSplitDistortAudioProcessorEditor)
};