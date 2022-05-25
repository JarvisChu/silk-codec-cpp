#ifndef _SILK_CODEC_H
#define _SILK_CODEC_H

#include <string>
#include <vector>
#include <stdint.h>

namespace SilkCodec {

// 错误码表
enum ErrCode {
    ErrCodeSuccess = 0,
    ErrCodeInvalidParam = 1, // 参数错误
    ErrCodeGetEncoderSizeFailed = 2, //
    ErrCodeGetDecoderSizeFailed = 3, // 
    ErrCodeInitEncoderFailed = 4,
    ErrCodeInitDecoderFailed = 5,
    ErrCodeUninitialized = 6, // 未初始化，请先调用InitEncoder/InitDecoder
    ErrCodeEncodeFailed = 7,
    ErrCodeDecodeFailed = 8,
    ErrCodeOpenFileFailed = 9,
    ErrCodeInvalidSilkFile = 10,
};

/* 
 *   Silk 编码的标准封装为20ms的语音分片，每个分片包含 片长 和 语音数据 两部分，其中 片长 为开头的2字节，表明后面语音数据的长度。
 *        即 package=[len+data]，Silk=[package1][package2]...
 *   Silk 文件的标准格式为：SILK_HEADER + 标准封装的SILK编码数据，其中 SILK_HEADER 固定为 #!SILK_V3
 */

// silk 编码器
class SilkEncoder {
public:
    explicit SilkEncoder();
    ~ SilkEncoder();

    /* 初始化 Silk 编码器
     * sampleRate: 采样率，支持 8000\16000\24000\32000\44100\48000
     * sampleBits: 采样的位深，支持 8\16\24\32
     * channelCnt: 通道数，支持 1\2\4\8
     * return: 0 成功
     */
    int Init(uint32_t sampleRate, uint32_t sampleBits, uint32_t channelCnt);

    /* 将 pcm 数据编码成 silk
     * pcmIn: 待编码的 pcm 数据
     * silkOut2Append: 编码后的 silk 数据会追加到 silkOut2Append 中，编码后的 silk 为标准的20ms分片的封装
     * bitRate: 设置silk编码音频的比特率，该参数可以控制silk的压缩率。e.g. 25000：约6倍压缩, 20000：约7倍压缩, 10000：约13倍压缩, 8000：约16倍压缩
     * return: 0 成功，成功时 silk 数据会被追加到 silkOut2Append 中
     */
    int Encode(const std::vector<uint8_t>& pcmIn, std::vector<uint8_t>& silkOut2Append, uint32_t bitRate = 10000);

    /* 将 pcm 语音文件编码成 silk 语音文件
     * pcmFilePath: 要编码的 pcm 语音文件的路径
     * silkFilePath: 要保存的 silk 语音文件路径
     * bitRate: 设置silk编码音频的比特率，该参数可以控制silk的压缩率。e.g. 25000：约6倍压缩, 20000：约7倍压缩, 10000：约13倍压缩, 8000：约16倍压缩
     */
    int EncodeFile(const std::string& pcmFilePath, const std::string& silkFilePath, uint32_t bitRate = 10000);

private:
    void *   m_pEncoder = nullptr;
    uint32_t m_sampleRate = 0;
    uint32_t m_sampleBits = 0;
    uint32_t m_channelCnt = 0;
 };

// silk 解码器
class SilkDecoder {
public:
    explicit SilkDecoder();
    ~ SilkDecoder();

    /* 初始化 Silk 解码器
     * sampleRate: 采样率，支持 8000\16000\24000\32000\44100\48000
     * return: 0 成功
     */
    int Init(uint32_t sampleRate);

    /* 将标准封装的 silk 数据解码成 pcm
     * silkIn: 待解码的 silk 数据，其格式为标准的20ms分片的封装
     * pcmOut2Append: 解码后的 pcm 数据会追加到 pcmOut2Append 中
     * return: 0 成功，成功时 pcm 数据会被追加到 pcmOut2Append 中
     */
    int Decode(const std::vector<uint8_t>& silkIn, std::vector<uint8_t>& pcmOut2Append);

    /* 将无封装的一个分片的 silk 数据解码成 pcm
     * silkIn: 待解码的 silk 数据，无封装，即无 片长 字段，为语音数据
     * pcmOut2Append: 解码后的 pcm 数据会追加到 pcmOut2Append 中
     * return: 0 成功，成功时 pcm 数据会被追加到 pcmOut2Append 中
     */
    int DecodeRaw(const std::vector<uint8_t>& silkIn, std::vector<uint8_t>& pcmOut2Append);

    /* 将 silk 语音文件解码成 pcm 语音文件
     * silkFilePath: 要解码的 silk 语音文件的路径
     * pcmFilePath: 要保存的 pcm 语音文件路径
     */
    int DecodeFile(const std::string& silkFilePath, const std::string& pcmFilePath);

private:
    void *   m_pDecoder = nullptr;
    uint32_t m_sampleRate = 0;
 };

};

#endif
