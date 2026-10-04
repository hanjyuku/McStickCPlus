//******************************************************************************
//	ESP-NOW関連クラス
//		2024/09/07: メインソースファイルより分離
//		2024/09/08: いろいろ調整
//		2025/05/25:	Tab5対応
//		2025/07/23:	Tab5でも ESP-NOWを使用できるように
//		2026/08/15: IDF 5.4からIDF 5.5への変更に伴う送信コールバック関数の引数修正
//		2026/08/16:	Tab5では ESP-NOWを使用できないので最小限のロジック無効化
//******************************************************************************
//*** コンパイルスイッチ
#define DEBUG_MODE						// デバッグモード

#if defined(DEBUG_MODE)
#include <M5Unified.h>					// デバッグモードでは必要
#endif	//defined(DEBUG_MODE)
#include "_my_espnow.h"

//------------------------------------------------------------------------------
//	staticメンバ変数の定義。宣言は hファイル
//------------------------------------------------------------------------------
// 自身の情報
uint8_t MyESPNOW::_macAddr[6 + 1];											// MACアドレス
// ピア情報
esp_now_peer_info_t MyESPNOW::_peerMulti;									// マルチキャスト用ピア情報
esp_now_peer_info_t MyESPNOW::_peerUnis[ESP_NOW_MAX_TOTAL_PEER_NUM - 1];	// ユニキャスト用ピア情報(マルチキャスト用を除き、最大19)
uint8_t MyESPNOW::_nickNames[ESP_NOW_MAX_TOTAL_PEER_NUM - 1][32];			// ニックネーム配列: インデックスは peerUnis[]と連動
int MyESPNOW::_peerUnisCnt;													// ユニキャスト用ピア情報数
int MyESPNOW::_slaveIdx;													// Slaveの Index

volatile bool MyESPNOW::_calledSent;										// OnSent()内で trueになる
volatile bool MyESPNOW::_calledRecv;										// OnRecv()内で trueになる

uint8_t MyESPNOW::_recvMAC[6 + 1];											// 送信元MACアドレス
uint8_t MyESPNOW::_recvData[250 + 1];										// 受信データ
int MyESPNOW::_recvLen;														// 受信データ長

uint8_t MyESPNOW::_sendMAC[6 + 1];											// 送信先MACアドレス
esp_now_send_status_t MyESPNOW::_sendSts;									// 送信コールバック関数に渡されるステータス

uint32_t MyESPNOW::_requestMacTime;											// Slaveへ MACアドレス要求を送信した時間(millis()をセット)

#if defined(DEBUG_MODE)
// デバッグ情報
uint8_t MyESPNOW::_log[MyESPNOW::_logLenMax];
int MyESPNOW::_logCnt;
#endif	//defined(DEBUG_MODE)

//******************************************************************************
//	コンストラクタ: Privateなのでクラス外からはインスタンス生成できません
//******************************************************************************
MyESPNOW::MyESPNOW(void)
{
}

MyESPNOW::~MyESPNOW(void)
{
}

//******************************************************************************
//	Publicなメソッド
//******************************************************************************
//------------------------------------------------------------------------------
//	初期化
//		IN	void
//	OUT:		true = 正常終了, false: エラー
//------------------------------------------------------------------------------
bool MyESPNOW::init(void)
{
#if defined(ARDUINO_M5STACK_TAB5)	// 今しばらくは Tab5での ESP-NOWは封印
	return(false);
#else	//defined(ARDUINO_M5STACK_TAB5)
	esp_err_t espErr;

#if defined(DEBUG_MODE)
	memset(_log, 0x00, sizeof((const char*)_log));
	M5.Log.printf("-------------------------------------------------------\n");
	M5.Log.printf("- init(): ESP-NOW 初期化処理\n");
	M5.Log.printf("-------------------------------------------------------\n");
#endif	//defined(DEBUG_MODE)

	//*** メンバー変数の初期化。コンストラクタで行わないのでここで
	{
		// 自身の情報
		memset(_macAddr, 0x00, sizeof((const char*)_macAddr));			// MACアドレス
		// ピア情報
		memset(&_peerMulti, 0x00, sizeof(_peerMulti));		// マルチキャスト用ピア情報
		memset(_peerUnis, 0x00, sizeof(_peerUnis));		// ユニキャスト用ピア情報(マルチキャスト用を除き、最大19)
		memset(_nickNames, 0x00, sizeof(_nickNames));		// ニックネーム配列: インデックスは peerUnis[]と連動
		_peerUnisCnt = 0;												// ユニキャスト用ピア情報数
		_slaveIdx = -1;													// Slaveの Index
		// コールバック関数呼び出しフラグ
		_calledSent = false;
		_calledRecv = false;
		// コールバック関数で使用する受信データ
		memset(_recvMAC, 0x00, sizeof(_recvMAC));				// 送信元MACアドレス
		memset(_recvData, 0x00, sizeof(_recvData));				// 受信データ
		_recvLen = 0;											// 受信データ長
		// コールバック関数で使用する送信ステータス
		uint8_t _sendMAC[6 + 1];								// 送信先MACアドレス
		_sendSts = ESP_NOW_SEND_SUCCESS;						// 送信コールバック関数に渡されるステータス

		// タイムアウト検出用
		_requestMacTime = 0;									// Slaveへ MACアドレス要求を送信した時間(millis()をセット)
#if defined(DEBUG_MODE)
		// デバッグ情報
		memset(_log, 0x00, sizeof(_log));
		_logCnt = 0;;
#endif	//defined(DEBUG_MODE)
	}

	//------------------------------------------------------
	//	初期化
	//------------------------------------------------------
	//*** 無線初期化・ESP-NOW初期化
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	espErr = esp_now_init();
#ifdef DEBUG_MODE
	M5.Log.printf("- esp_now_init() = %d\n", espErr);
#endif	//DEBUG_MODE
	if(espErr != ESP_OK)
	{
		return(false);
	}

	//------------------------------------------------------
	//	自身のMACアドレス取得
	//------------------------------------------------------
	memset(_macAddr, 0x00, sizeof((const char*)_macAddr));
	esp_read_mac(_macAddr, ESP_MAC_WIFI_STA);

	//------------------------------------------------------
	//	送受信コールバック関数登録
	//------------------------------------------------------
	//*** 送信コールバック関数登録
	espErr = esp_now_register_send_cb(OnSent);
#ifdef DEBUG_MODE
	M5.Log.printf("- esp_now_register_send_cb() = %d\n", espErr);
#endif	//DEBUG_MODE
	if(espErr != ESP_OK)
	{
		return(false);
	}

	//*** 受信コールバック関数登録
	espErr = esp_now_register_recv_cb(OnRecv);
#ifdef DEBUG_MODE
	M5.Log.printf("- esp_now_register_recv_cb() = %d\n", espErr);
#endif	//DEBUG_MODE
	if(espErr != ESP_OK)
	{
		// 送信コールバック解除
		espErr = esp_now_unregister_send_cb();
#ifdef DEBUG_MODE
		M5.Log.printf("- esp_now_unregister_send_cb() = %d\n", espErr);
#endif	//DEBUG_MODE

		return(false);
	}

	//------------------------------------------------------
	//	ピア情報初期化
	//------------------------------------------------------
	//*** マルチキャスト用ピア情報初期化
	memset(&_peerMulti, 0, sizeof(_peerMulti));
	memset(&(_peerMulti.peer_addr), 0xFF, sizeof(_peerMulti.peer_addr));
	espErr = esp_now_add_peer(&_peerMulti);
#ifdef DEBUG_MODE
	M5.Log.printf("- マルチキャスト用 esp_now_add_peer() = %d\n", espErr);
#endif	//DEBUG_MODE
	if(espErr != ESP_OK)
	{
#ifdef DEBUG_MODE
		M5.Log.printf("- マルチキャスト用 esp_now_add_peer()がエラーになったので、コールバック解除\n");
#endif	//DEBUG_MODE
		// 受信コールバック解除
		espErr = esp_now_unregister_recv_cb();
#ifdef DEBUG_MODE
		M5.Log.printf("- esp_now_unregister_recv_cb() = %d\n", espErr);
#endif	//DEBUG_MODE

		// 送信コールバック解除
		espErr = esp_now_unregister_send_cb();
#ifdef DEBUG_MODE
		M5.Log.printf("- esp_now_unregister_send_cb() = %d\n", espErr);
#endif	//DEBUG_MODE

		return(false);
	}

	//*** ユニキャスト用ピア情報初期化、ここで登録はしない
	memset(_peerUnis, 0, sizeof(_peerUnis));

	//------------------------------------------------------
	//	ニックネーム配列初期化
	//------------------------------------------------------
	memset(_nickNames, 0x00, sizeof(_nickNames));

//	vPeerUnis.clear();

	// ここまで来たら正常終了
	return(true);
#endif	//defined(ARDUINO_M5STACK_TAB5)
}

//------------------------------------------------------------------------------
//	自身のMACアドレス受け渡し
//		IN	mac*:	受信データ格納先へのポインター
//		OUT:		void
//------------------------------------------------------------------------------
void MyESPNOW::getMac(uint8_t* mac)
{
	memcpy(mac, _macAddr, 6);
}

//------------------------------------------------------------------------------
//	受信データ受け渡し
//		IN	mac*:	受信データ格納先へのポインター
//			data*:	受信データ格納先へのポインター
//		OUT:		データ長
//------------------------------------------------------------------------------
int MyESPNOW::recv(uint8_t* mac, uint8_t* data)
{
	memcpy(mac, _recvMAC, 6);
	memcpy(data, _recvData, _recvLen);

	return(_recvLen);
}

//------------------------------------------------------------------------------
//	送信処理
//		IN	idx:	ピアテーブルの登録インデックスNo
//			data*:	送信データへのポインター
//			len:	送信データバイト数
//		OUT:		true = 正常終了, false: エラー
//------------------------------------------------------------------------------
bool MyESPNOW::send(int idx, uint8_t* data, int len)
{
#if defined(ARDUINO_M5STACK_TAB5)	// 今しばらくは Tab5での ESP-NOWは封印
	return(false);
#else	//defined(ARDUINO_M5STACK_TAB5)
	esp_err_t espErr;
	uint8_t* addr = ((idx == -1) ? _peerMulti.peer_addr : _peerUnis[idx].peer_addr);	// 送信先MACアドレス
	uint8_t* nickName = getNickName(idx);												// 送信先ニックネーム

	// 送信ループバック判定を有効にしてデータ送信
	MyESPNOW::_calledSent = false;
	espErr = esp_now_send(addr, data, (len + 1));
	// 送信が成功したらコールバック関数が呼ばれるのを待つ
	if(espErr == ESP_OK)
	{
		espErr = ESP_ERR_ESPNOW_IF;	//************************* あとでタイムアウトにふさわしいエラーコードをセット
		// 1000ms以内に送信コールバック関数が呼ばれなければエラー
		uint32_t endMillis = (millis() + TIMEOUT_SEND_CALLBACK);
		while(millis() < endMillis)
		{
			if(_calledSent)
			{
				espErr = ESP_OK;
				break;
			}
		}
	}
#ifdef DEBUG_MODE
	M5.Log.printf("\t- ESP-NOW送信処理: %s(%d), 送信先: [%s](idx=%d)[%02X:%02X:%02X:%02X:%02X:%02X], 送信データ長: %dbytes\n",
		((espErr == ESP_OK) ? "成功" : "失敗"), espErr,
		nickName, idx, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
		len
	);
#endif	//DEBUG_MODE

	return((espErr == ESP_OK));
#endif	//defined(ARDUINO_M5STACK_TAB5)
}

//------------------------------------------------------------------------------
//	ピア登録チェック
//		IN	addr*:	MACアドレスへのポインター
//		OUT:		-1: 未登録, 0～: インデックスNo
//------------------------------------------------------------------------------
int MyESPNOW::checkPeerRegisterd(uint8_t* addr)
{
	int8_t idx = -1;

	// 指定された MACアドレスがピアに登録済みかどうかチェック
	for(int ii = 0; ii < _peerUnisCnt; ii++)
	{
		if(!memcmp(_peerUnis[ii].peer_addr, addr, sizeof(_peerUnis[ii].peer_addr)))
		{
			idx = ii;
			break;
		}
	}

	return(idx);
}

//------------------------------------------------------------------------------
//	ピア登録
//		IN	addr*:	MACアドレスへのポインター
//		OUT:		-1: エラー, 0～: 登録インデックスNo
//------------------------------------------------------------------------------
//	esp_now_add_peer()のエラー
//		12388: #define ESP_ERR_ESPNOW_BASE         (ESP_ERR_WIFI_BASE + 100) /*!< ESPNOW error number base. */
//		12389: #define ESP_ERR_ESPNOW_NOT_INIT     (ESP_ERR_ESPNOW_BASE + 1) /*!< ESPNOW is not initialized. */
//		12390: #define ESP_ERR_ESPNOW_ARG          (ESP_ERR_ESPNOW_BASE + 2) /*!< Invalid argument */
//		12391: #define ESP_ERR_ESPNOW_NO_MEM       (ESP_ERR_ESPNOW_BASE + 3) /*!< Out of memory */
//		12392: #define ESP_ERR_ESPNOW_FULL         (ESP_ERR_ESPNOW_BASE + 4) /*!< ESPNOW peer list is full */
//		12393: #define ESP_ERR_ESPNOW_NOT_FOUND    (ESP_ERR_ESPNOW_BASE + 5) /*!< ESPNOW peer is not found */
//		12394: #define ESP_ERR_ESPNOW_INTERNAL     (ESP_ERR_ESPNOW_BASE + 6) /*!< Internal error */
//		12395: #define ESP_ERR_ESPNOW_EXIST        (ESP_ERR_ESPNOW_BASE + 7) /*!< ESPNOW peer has existed */
//		12396: #define ESP_ERR_ESPNOW_IF           (ESP_ERR_ESPNOW_BASE + 8) /*!< Interface error */
//------------------------------------------------------------------------------
int MyESPNOW::registPeer(uint8_t* addr)
{
#if defined(ARDUINO_M5STACK_TAB5)	// 今しばらくは Tab5での ESP-NOWは封印
	return(-1);
#else	//defined(ARDUINO_M5STACK_TAB5)
	int8_t idx = -1;
	esp_err_t espErr;

	// ピア情報初期化
	memset(&_peerUnis[_peerUnisCnt], 0x00, sizeof(esp_now_peer_info_t));
	memcpy(_peerUnis[_peerUnisCnt].peer_addr, addr, sizeof(_peerUnis[_peerUnisCnt].peer_addr));

	// ピア登録
	if((espErr = esp_now_add_peer(&_peerUnis[_peerUnisCnt])) == ESP_OK)
	{
		idx = _peerUnisCnt;
		// ピア登録数カウントアップ
		_peerUnisCnt++;
	}
#ifdef DEBUG_MODE
	M5.Log.printf("- MACアドレス[%02X:%02X:%02X:%02X:%02X:%02X]の新規ピア登録：%s(Index = %d)\n",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
		((espErr == ESP_OK) ? "成功" : "失敗"), _peerUnisCnt
	);
#endif	//DEBUG_MODE

	return(idx);
#endif	//defined(ARDUINO_M5STACK_TAB5)	// 今しばらくは Tab5での ESP-NOWは封印
}

//------------------------------------------------------------------------------
//	ニックネーム登録
//		IN	idx:	ピア情報インデックスNo
//			name*:	ニックネームへのポインター(NULL終端文字列、最大16文字)
//		OUT:		-1: エラー, 0～: 登録インデックスNo
//------------------------------------------------------------------------------
int MyESPNOW::registNickName(int idx, uint8_t* name)
{
	// ニックネーム登録
	memset(&_nickNames[idx], 0x00, sizeof(&_nickNames[idx]));
	memcpy(&_nickNames[idx], name, strlen((const char*)name));
#ifdef DEBUG_MODE
	M5.Log.printf("- ニックネーム[%d] = [%s]登録\n", idx, name);
#endif	//DEBUG_MODE

	return(idx);
}

//------------------------------------------------------------------------------
//	ニックネーム取得
//		IN	idx:	ピア情報インデックスNo
//		OUT:		ニックネームへのポインター(NULL終端文字列、最大16文字)
//------------------------------------------------------------------------------
uint8_t* MyESPNOW::getNickName(int idx)
{
	uint8_t* cRet = (uint8_t*)"Unknown";

	if(idx < 0)
	{
		return(cRet);
	}
	if(*_nickNames[idx] == NULL)
	{
		return(cRet);
	}
	// ここまでくればOK
	cRet = (uint8_t*)(&_nickNames[idx]);

	return(cRet);
}

//******************************************************************************
//	Privateで Staticなメソッド
//******************************************************************************
//------------------------------------------------------------------------------
//	送信コールバック関数
//------------------------------------------------------------------------------
#if true	// IDF 5.4からIDF 5.5への変更に伴う変更
void MyESPNOW::OnSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t sts)
#else
void MyESPNOW::OnSent(const uint8_t* addr, esp_now_send_status_t sts)
#endif		// ボードパッケージの v2.x→ v3.xアップデートに伴う変更
{
	_calledSent = true;			// falseにするのはデータを送信する直前

	// 送信先MACアドレス保存: MACアドレスが 6バイト以外のことはない前提
	memset(_sendMAC, 0x00, sizeof(_sendMAC));
#if true	// ボードパッケージの v2.x→ v3.xアップデートに伴う変更
	memcpy(_sendMAC, tx_info->src_addr, 6);
#else
	memcpy(_sendMAC, addr, 6);
#endif		// ボードパッケージの v2.x→ v3.xアップデートに伴う変更
	// 送信ステータスを保存
	_sendSts = sts;
#ifdef DEBUG_MODE
	if(sts)
	{
		M5.Log.printf("+++ OnSent(): ESP-NOW 送信コールバック sts = %d(MAC[%02X:%02X:%02X:%02X:%02X:%02X]) +++\n",
			sts, 
			_sendMAC[0], _sendMAC[1], _sendMAC[2], _sendMAC[3], _sendMAC[4], _sendMAC[5]
		);
	}
#endif	//DEBUG_MODE
}

//------------------------------------------------------------------------------
//	受信コールバック関数
//------------------------------------------------------------------------------
//void MyESPNOW::OnRecv(const uint8_t* addr, const uint8_t* data, int len)
void MyESPNOW::OnRecv(const esp_now_recv_info* info, const unsigned char* data, int len)
{
	_calledRecv = true;			// falseにするのは受信データを処理してから

	// 送信元MACアドレス保存: MACアドレスが 6バイト以外のことはない前提
	memset(_recvMAC, 0x00, sizeof(_recvMAC));
	memcpy(_recvMAC, info->src_addr, 6);
	// 受信データ・受信データ数保存
	memset(_recvData, 0x00, sizeof(_recvData));
	memcpy(_recvData, data, len);
	_recvLen = len;
#ifdef DEBUG_MODE
	{
		M5.Log.printf("+++ OnRecv(): ESP-NOW 受信コールバック %dbytes Recieved(cmd = 0x%02X, mode = 0x%02X, MAC[%02X:%02X:%02X:%02X:%02X:%02X]) +++\n",
			len, *(data + 0), *(data + 1), 
			*(_recvMAC + 0), *(_recvMAC + 1), *(_recvMAC + 2), *(_recvMAC + 3), *(_recvMAC + 4), *(_recvMAC + 5)
		);
	}
#endif	//DEBUG_MODE
}
