//******************************************************************************
//	ESP-NOW関連クラス
//		2024/09/07: メインソースファイルより分離
//		2024/09/08: 全メンバー関数/変数を staticに
//		2024/09/08: いろいろ調整
//		2025/05/25:	Tab5対応
//		2025/07/23:	Tab5でも ESP-NOWを使用できるように
//		2026/08/15: IDF 5.4からIDF 5.5への変更に伴う送信コールバック関数の引数修正
//------------------------------------------------------------------------------
//	Notice:
//		staticなメソッドのみなのでインスタンスを生成せずに使用すること。
//		コンストラクタを Privaeにしているので、クラス外でのインスタンス化は不可能。
//		メンバー変数もすべて Privateなので、クラス外からは Setter/Getterを使用すること。
//******************************************************************************
#ifndef	__MY_ESPNOW_H__
#define	__MY_ESPNOW_H__

#include <WiFi.h>						// for WiFi
#include <esp_now.h>					// for ESP-NOW
#include <esp_mac.h>					// 2025/05/30以降、インクルードしないと esp_read_mac()が未定義となる

//------------------------------------------------------------------------------
//	ESP-NOW処理をまとめたクラス
//------------------------------------------------------------------------------
class MyESPNOW
{
public:
	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	static bool init(void);									// 初期化
	static void getMac(uint8_t* mac);						// 自身のMAC取得
	static int recv(uint8_t* mac, uint8_t* data);			// 受信(後処理)
	static bool send(int idx, uint8_t* data, int len);		// 送信
	static int checkPeerRegisterd(uint8_t* addr);			// ピア登録チェック
	static int registPeer(uint8_t* addr);					// ピア登録
	static int registNickName(int idx, uint8_t* name);		// ニックネーム登録
	static uint8_t* getNickName(int idx);					// ニックネーム取得

	//------------------------------------------------------------------------------
	//	プライベートメンバー変数用Setter/Getter
	//------------------------------------------------------------------------------
	// 送信コールバック関数が呼ばれたか
	static void setCalledSent(bool flag){_calledSent = flag;}
	static bool isCalledSent(void){return(_calledSent);}

	// 受信コールバック関数が呼ばれたか
	static void setCalledRecv(bool flag){_calledRecv = flag;}
	static bool isCalledRecv(void){return(_calledRecv);}

private:
	//------------------------------------------------------------------------------
	//	Privateメソッド
	//------------------------------------------------------------------------------
	//*** インスタンス化はさせない
	MyESPNOW(void);		// コンストラクタ
	~MyESPNOW(void);	// デストラクタ

	// コールバックも staticでないといけない
//	static void OnSent(const uint8_t* addr, esp_now_send_status_t sts);		// esp_now_send()実行後にコールバックされる
//	static void OnSent(const unsigned char* addr, esp_now_send_status_t sts);		// esp_now_send()実行後にコールバックされる	2025/05/30
	static void OnSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t sts);	// esp_now_send()実行後にコールバックされる	2026/08/15
//	static void OnRecv(const uint8_t* addr, const uint8_t* data, int len);	// コールバックされたら受信データを取得
	static void OnRecv(const esp_now_recv_info* info, const unsigned char* data, int len);	// コールバックされたら受信データを取得
//	static void OnRecv(const esp_now_recv_info* info, const unsigned char* data, int len);	// コールバックされたら受信データを取得		2025/05/30

	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	//	Staticなメソッドで使用する変数もStatic変数でなければならない
	static const uint32_t TIMEOUT_SEND_CALLBACK = 1000;	// ESP-NOW通信での応答待ち時間(ms)

	//*** staticメンバ変数の宣言。ここで初期化はできない。定義・初期化は cppファイルで行う。
	// 自身の情報
	static uint8_t _macAddr[];					// MACアドレス
	// ピア情報
	static esp_now_peer_info_t _peerMulti;		// マルチキャスト用ピア情報
	static esp_now_peer_info_t _peerUnis[];		// ユニキャスト用ピア情報(マルチキャスト用を除き、最大19)
	static uint8_t _nickNames[][32];			// ニックネーム配列: インデックスは peerUnis[]と連動
	static int _peerUnisCnt;					// ユニキャスト用ピア情報数
	static int _slaveIdx;						// Slaveの Index

	static volatile bool _calledSent;			// OnSent()内で trueになる
	static volatile bool _calledRecv;			// OnRecv()内で trueになる

	// コールバック関数で使用する受信データ
	static uint8_t _recvMAC[];					// 送信元MACアドレス
	static uint8_t _recvData[];					// 受信データ
	static int _recvLen;						// 受信データ長

	static uint32_t _requestMacTime;			// Slaveへ MACアドレス要求を送信した時間(millis()をセット)
	// コールバック関数で使用する送信ステータス
	static uint8_t _sendMAC[];					// 送信先MACアドレス
	static esp_now_send_status_t _sendSts;		// 送信コールバック関数に渡されるステータス

#if defined(DEBUG_MODE)
	// デバッグ情報
//	static const int _logLenMax = (1024 * 100);	// エラー: [DRAM segment data does not fit.]が発生。
	static const int _logLenMax = (1024 * 20);	// 「ボードライブラリのバージョンアップにより、自由に使えるメモリ量が減りました。」とのこと。参考: https://lang-ship.com/blog/work/m5stickc-mic-2/
	static uint8_t _log[];
	static int _logCnt;
#endif	//defined(DEBUG_MODE)
};
#endif	//__MY_ESPNOW_H__
