//**************************************************************************************************
//	メディアコントロール
//		2025/07/14:	メディアコントロールサーバーからメディアコントロールに改名。略称Mc
//		2025/07/16:	Tab5で Wifi.begin()を飛び出してもリセットかからなくなったので、制限解除
//		2025/07/20:	USB-OTGモードのビルド時判定の正常化
//		2025/07/23:	機種別に機能制限を行う
//		2025/08/02:	AtomLite/StickCを対象外に
//		2025/08/02:	StickCPlus系でデバイスは Hatとハットにつなげた I2Cデバイスのみとする。Port-Aは使用しない。
//		2025/08/05:	AtomS3での LED制御は neopixelWrite()ではなく AtomS3.dis.～を使用する。
//		2025/08/06:	Stick系の HY2.0-4Pはデュアルボタンユニットで使用するように変更
//		2025/08/07: CORE.INK対応
//		2025/08/07:	StickCを再び対象に。ただし画面表示は StickCPlusのまま。
//		2025/08/30:	ESP-NOWの相互通信を正常化
//		2025/08/31:	USB制御をサブルーチン化
//		2025/09/19:	各種サブルーチンの最適化
//		2025/09/22:	I2Cデバイス制御クラスを各デバイスごとに派生クラスを持たせるように変更(エンコーダーユニット、エンコーダーHAT)
//		2025/09/23:	I2Cデバイス関連の設定を各デバイスごとのクラスに移動
//		2025/09/23:	_McSub.inoからデバイス関連のみを抜き出して _McSubDevice.inoへ
//		2026/08/23:	AtomS3でビルドエラー対策に D:\LABO\M5\libraries\M5AtomS3\src\utility\LedDisplay.cppを修正しているので注意
//		2026/09/05:	ENV.Ⅲユニット対応
//--------------------------------------------------------------------------------------------------
//	Memo:
//		Atom Lite:		動作対象外。
//		Atom S3:		Slave可能。
//		Atom S3U:		USG-OTGモードでのみ Slaveとして動作する。他に Slaveが存在した場合は LEDマゼンタ点滅で何もしない。
//						デバイスは Port-Aの I2Cデバイス(Mini OLEDユニット)のみ対応。
//		StickC:			自身が Slaveになったら単なる時計として動作し、デバイス操作をチェックしない。
//						デバイスは Hatとハットにつなげた I2Cデバイスのみ対応。
//		StickCPlus:		↑
//		StickCPlus2:	↑
//		Core1:			自身が Slaveになったら単なる時計として動作し、デバイス操作をチェックしない
//		Core2:			↑
//		CoreS3:			Slave可能
//		Dial:			Slave可能
//		Tab5:			未確認
//--------------------------------------------------------------------------------------------------
//	Notice:
//		USBデバイスとして動作させるため、ESP32-S3搭載機種(M5Dial、AtomS3、Core3SE)のみを対象に。
//		↑ 送信側テストのため、一時的にこの制限を解除しています
//--------------------------------------------------------------------------------------------------
//	Notice(I2C):
//		外部I2C(Ex_I2C)は明示的に begin()する必要がある
//		内部I2C(In_I2C)は M-BUS接続、外部I2C(Ex_I2C)はポートA接続。
//		よってモジュールは内部I2C接続、ユニットは外部I2C接続となる。
//		ただし内部I2Cの使用は Core2/Cores3のみにとどめること。
//			Core1:	どちらも GPIO 21/22を利用しているが In_I2Cの利用は控えること
//			Core2:	前者は GPIO 21/22、後者は GPIO 32/33を利用
//			CoreS3:	前者は GPIO 12/11、後者は GPIO 2/1を利用
//			StickC:	前者は GPIO 21/22、後者は HY2.0-4Pが GPIO 32/33、Ext.8Pが GPIO 0/26を利用
//			CPlus:	↑
//			CPlus2:	↑
//			Atom:	前者は GPIO 25/21(底面ピンにも出ている)、後者は GPIO 26/32を利用
//			AtomS3:	前者は GPIO 38/39(底面ピンにも出ている)、後者は GPIO 2/1を利用
//			Dial:	前者は GPIO 11/12、後者は GPIO 13/15を利用
//			Tab5:	未調査
//	----------------------------------------------------------------------------
//		M5Unifedの I2C_Classクラスを使用した後だと Wireクラスの動作が不安定になる
//		I2C_Classクラスと Wireクラスの同時使用は不可。
//--------------------------------------------------------------------------------------------------
//	Notice(Core):
//		エンコーダーモジュール/HMIモジュールは I2Cのピンとして GPIO 21/22を
//		利用しているので、M5Unifiedでは In_I2Cに接続されている。
//		Core2ではポートAは GPIO 32/33を利用して Ex_I2Cに接続されているので、
//		モジュールとユニット両方の I2Cを使用する際には注意が必要。 → 同時に使えます。
//--------------------------------------------------------------------------------------------------
//	Notice(CoreS3, CoreS3SE):
//		CoreS3対応にするには、M5.begin()の代わりに M5CoreS3.beign()を使用すればいい。
//		M5インスタンスと M5CoreS3インスタンスは同じ名前空間にあり、M5CoreS3.begin()は
//		M5Unifiedクラスの begin()を呼び出してから Mic_Classクラスなどを
//		初期化している。
//		M5CoreS3クラスでは begin()と update()のみ疑似 overrideしているが、
//		update()は M5.update()を呼び出しているだけなので、
//		実質 begin()のみ M5CoreS3クラスのものを使用すればいい。
//	----------------------------------------------------------------------------
//		M5CoreS3クラスは GC0308クラス(CMOSカメラ)と LTR5XXクラス(照度・近接センサ)の
//		インスタンスを生成しているだけなので、CoreS3SEでは M5Unifiedクラスそのまま
//		使用する方がメモリ節約になるかも。← 逆に大きくなりました。
//	----------------------------------------------------------------------------
//		GPIO 11/12が Wire1に割り当たっている時に I2Cスキャンを行うと先頭8アドレスは 2(NAK)、
//		以降は 5(TimeOut)を返す。
//		ただし、該当ピンを Wireに割り当てなおすと I2Cスキャンは正常に行われる。
//		M5Unifiedのソース(https://github.com/m5stack/M5Unified/blob/master/src/utility/I2C_Class.cpp)に
//		「ESP32S3ではアドレス0~7をスキャン対象に含めると動作が停止する」との記載があったので、
//		対象外にしても動作は同じだった。
//--------------------------------------------------------------------------------------------------
//	Notice(StickC Plus2):
//		CPlus2対応にするには、M5.begin()の代わりに M5StickCPlus2.beign()を使用すればいい。
//		M5インスタンスと M5StickCPlus2インスタンスは同じ名前空間にあり、M5StickCPlus2.begin()は
//		M5Unifiedクラスの begin()を呼び出しているのみ。
//		M5StickCPlus2クラスでは begin()のみ疑似 overrideしているが、
//		begin()は M5.update()を呼び出しているだけなので、
//		M5StickCPlus2クラスは使用しなくても問題ない。
//--------------------------------------------------------------------------------------------------
//	Notice(M5Dial):
//		M5Dial対応にするには、M5.begin()の代わりに M5Dial.beign()、エンコーダーと
//		RFIDの利用時に M5インスタンスの代わりに M5Dialインスタンスを使用すればいい。
//		M5インスタンスと M5Dialインスタンスは同じ名前空間にあり、M5Dial.begin()は
//		M5Unifiedクラスの begin()を呼び出してから ENCODERクラスと MFRC522クラスを
//		初期化している。
//		M5_Dialクラスでは begin()と update()のみ疑似 overrideしているが、
//		update()は M5.update()を呼び出しているだけなので、
//		実質 begin()のみ M5_DIALクラスのものを使用すればいい。
//	----------------------------------------------------------------------------
//		GPIO 11/12が Wire1に割り当たっている時に I2Cスキャンを行うと先頭8アドレスは 2(NAK)、
//		以降は 5(TimeOut)を返す。
//		ただし、該当ピンを Wireに割り当てなおすと I2Cスキャンは正常に行われる。
//		M5Unifiedのソース(https://github.com/m5stack/M5Unified/blob/master/src/utility/I2C_Class.cpp)に
//		「ESP32S3ではアドレス0~7をスキャン対象に含めると動作が停止する」との記載があったので
//		対象外にしてみたが、動作は同じだった。
//--------------------------------------------------------------------------------------------------
//	Notice(AtomS3):
//		AtomS3対応にするには、M5.begin()の代わりに AtomS3.beign()を、
//		M5.Update()の代わりに AtomS3.update()を使用すればいい。
//		M5インスタンスと AtomS3インスタンスは同じ名前空間にあり、AtomS3.begin()は
//		M5Unifiedクラスの begin()を呼び出してから LedDisplayクラスを初期化している。
//		M5AtomS3クラスでは begin()と update()のみ疑似 overrideしている。
//	----------------------------------------------------------------------------
//		GPIO 38/39が Wire1に割り当たっている時に I2Cスキャンを行うと先頭8アドレスは 2(NAK)、
//		以降は 5(TimeOut)を返す。
//		ただし、該当ピンを Wireに割り当てなおすと I2Cスキャンは正常に行われる。
//		M5Unifiedのソース(https://github.com/m5stack/M5Unified/blob/master/src/utility/I2C_Class.cpp)に
//		「ESP32S3ではアドレス0~7をスキャン対象に含めると動作が停止する」との記載があったので
//		対象外にしてみたが、動作は同じだった。
//**************************************************************************************************
//*** AtomLiteは対象外
#if defined(ARDUINO_M5STACK_ATOM)
#error	"********** Atom Lite is not supported. **********"
#endif	//defined(ARDUINO_M5STACK_ATOM)

//*** コンパイルスイッチ
#define DEBUG_MODE											// デバッグモード
#define USE_I2C_DEVICE_CLASS								// Wireクラスを使わず I2C_Class/I2C_Deviceクラスを使う。

#undef USE_USB_OTG
#if defined(ARDUINO_USB_MODE)
#if ARDUINO_USB_MODE == 0									// ビルドスイッチの「ARDUINO_USB_MODE」より、USB-OTGモード設定
#define USE_USB_OTG
#else
#warning	"********** ARDUINO_USB_MODE != 0 **********"
#endif	//ARDUINO_USB_MODE = 0
#else
#warning	"********** Not Defiend ARDUINO_USB_MODE **********"
#endif	//defined(ARDUINO_USB_MODE)

//*** ヘッダーインクルード
#include <Wire.h>											// Wire.hは M5Unified.hより先にインクルードする必要がある
//#include <M5UnitGLASS2.h>									// Glass2ユニット使用時には M5Unified.hより先にインクルードする必要がある
//#include <M5UnitMiniOLED.h>								// Mini OLEDユニットをシステムに任せて使用する際には M5Unified.hより先にインクルードする必要がある

#if defined(ARDUINO_M5STACK_CORES3)							// for CoreS3
#include <M5CoreS3.h>
#elif defined(M5STACK_STICKC_PLUS2)							// for StickC Plus2
#include <M5StickCPlus2.h>
#elif defined(ARDUINO_M5STACK_DIAL)							// for Dial
#include <M5Dial.h>
#elif defined(ARDUINO_M5STACK_ATOMS3)						// for AtomS3
#include <M5AtomS3.h>
#else														// for Core/Core2/StickC/StickCPlus/Tab5
#include <M5Unified.h>
#endif	//defined(ARDUINO_M5STACK_DIAL)

#include <WiFi.h>											// for WiFi
#include <esp_now.h>										// for ESP-NOW

//*** USB-OTGモードなら USBデバイスとして動作するために必要なヘッダーファイルをインクルード、インスタンス生成
#include <USBHIDConsumerControl.h>							// キーコード define値参照に必要
#if defined(USE_USB_OTG)
#include <USB.h>
USBHIDConsumerControl consumerControl;						// HID 準拠コンシュマー制御デバイス
#endif	//defined(USE_USB_OTG)

//*** Mini OLEDユニットを使用する場合、インクルードする
#include <M5UnitMiniOLED.h>

//*** Core系の場合、LED制御を行う
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
#include <_my_EspEasyLED.h>									// for NeoPixel LED
#endif

//*** 独自ヘッダーファイルインクルード
#include "_my_i2c.h"										// 独自I2Cデバイス管理クラス
#include "_my_class.h"										// 独自変数クラス
#include "_my_espnow.h"										// 独自ESP-NOWクラス

//*** 独自環境変数
#define TO_STRING(x) #x
#define EXPAND(x) TO_STRING(x)

//*** 独自変数
//*** _my_class.hに分離
MY my;

//*** Mini OLEDユニットを使用する場合
#if defined(__M5GFX_M5UNITMINIOLED__)
M5UnitMiniOLED* pMiniOLED;
M5Canvas* pCanvas;
#endif	//defined(__M5GFX_M5UNITMINIOLED__)

//*** LED制御を行う場合
#if defined(__EspEasyLED_H__)
#define BRIGHT 40
EspEasyLED led(my.PIN_EXT_RGBLED, my.LED_EXT_CNT, BRIGHT);
#endif	//defined(__EspEasyLED_H__)

//**************************************************************************************************
//	初期化
//**************************************************************************************************
void setup(void)
{
	bool bRet = false;
	bool bRet2 = false;

	MY::MEDIA_CONTROL mode;

	//------------------------------------------------------
	//	(10) システム初期化・電源周り初期化
	//------------------------------------------------------
	auto cfg = M5.config();
#if defined(ARDUINO_M5STACK_CORES3)							// for CoreS3
	CoreS3.begin(cfg);
#elif defined(M5STACK_STICKC_PLUS2)							// for StickC Plus2
	StickCP2.begin(cfg);
#elif defined(ARDUINO_M5STACK_DIAL)							// for Dial
	M5Dial.begin(cfg, true, true);								// エンコーダー：有効、RFID：有効
#elif defined(ARDUINO_M5STACK_ATOMS3)						// for AtomS3
	AtomS3.begin(cfg, true);									// LED：有効
#else														// for Core/Core2/StickC/Tab5
	M5.begin(cfg);
#endif	//defined(ARDUINO_M5STACK_DIAL)
	M5.Power.begin();

	//--------------------------------------------------------------------------
	//	(20) ログ出力用 Serial初期化
	//--------------------------------------------------------------------------
	{
		// (20-10) Serial
#ifdef DEBUG_MODE
		M5.Log.printf("(20-10) Serial initialize...\n");
#endif	//DBUGE_MODE
#if defined(ARDUINO_M5STACK_ATOMS3)
		Serial.begin(my.SERIAL_SPEED);
#else
//		Serial.begin(SERIAL_SPEED, SERIAL_SETTING);
		Serial.begin(my.SERIAL_SPEED);							// USB OTGモードにすると Serialが USBに割り当てられ、Serial::begin()の代わりに USBCDC::begin()となる。第2引数が無しとなる。
#endif
		// (20-20) 機種間通信用 Serial2初期化
#ifdef DEBUG_MODE
		M5.Log.printf("(20-20) Serial2 initialize...\n");
#endif	//DBUGE_MODE
		Serial2.begin(my.SERIAL2_SPEED, my.SERIAL2_SETTING, my.PIN_EXT_RX2, my.PIN_EXT_TX2);
	}

#ifdef DEBUG_MODE
	delay(1000);											// ちょっと待たないとシリアルログの最初が PCで読み取れない。
	M5.Log.printf("\n\n\n\n\n");
	M5.Log.printf("*******************************************************\n");
	M5.Log.printf("- setup() : start\n");
	M5.Log.printf("*******************************************************\n");
#endif	//DEBUG_MODE

	//--------------------------------------------------------------------------
	//	(30) ボード情報の取得
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("(30) ボード情報取得\n");
#endif	//DEBUG_MODE
		// ビルド時のボード名define値から自身のニックネーム取得
		memset(my.nickName, 0x00, sizeof(my.nickName));
		char* pTmp = EXPAND(ARDUINO_BOARD);
		// 先頭が[”](ダブルクォーテーション)であれば除去
		if(*pTmp == '"')
		{
			pTmp++;
		}
		memcpy(my.nickName, pTmp, strlen(pTmp));
		// 最後が[”](ダブルクォーテーション)であれば除去
		if(*(my.nickName + strlen(my.nickName) - 1) == '"')
		{
			*(my.nickName + strlen(my.nickName) - 1) = 0x00;
		}

		// 動的ボードの種類/インデックス取得
		my.boardType = M5.getBoard();
		my.dispIdx = M5.getDisplayIndex(my.boardType);

#ifdef DEBUG_MODE
		M5.Log.printf("\t静的ボード種類 :[%s]\n", my.nickName);
		M5.Log.printf("\t動的ボード種類 :[%s](type = %d), idx = %d\n", getBoardName(my.boardType), my.boardType, my.dispIdx);
#endif	//DEBUG_MODE
	}

	//--------------------------------------------------------------------------
	//	(40) 画面初期化
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(40) 画面初期化\n");
#endif	//DEBUG_MODE
		//*** (40-10) 表示エリアの座標計算
		bRet = calcCoordinate();
		//*** (40-20) 画面初期化・初期時計表示
		M5.Display.init();
#if defined(ARDUINO_M5STACK_TAB5)
//		M5.Display.setRotation(3);							// Tab5の時のみ、横長(コネクタが左側)に
#endif	//defined(ARDUINO_M5STACK_TAB5)
		M5.Display.fillScreen(my.COLOR_BLACK);
		// 時計表示
		drawClock(true);

#ifdef DEBUG_MODE
		// for 画面ログ表示
		M5.Display.setFont(&fonts::Font0);
		M5.Display.setTextSize(1);
		M5.Display.setTextColor(my.COLOR_FONT_INIT, my.COLOR_BACK_INIT);
		M5.Display.setCursor(0, 60);
#endif	//DEBUG_MODE
	}

#if defined(ARDUINO_M5STACK_ATOMS3)
	//--------------------------------------------------------------------------
	//	(50) AtomS3Uの場合のみ、RGB LED制御を行う
	//--------------------------------------------------------------------------
	if(my.boardType == m5::board_t::board_M5AtomS3U)
	{
		static int colorTable[] = { my.LEDCOLOR_OFF, my.LEDCOLOR_BLUE, my.LEDCOLOR_RED, my.LEDCOLOR_MAGENTA, my.LEDCOLOR_GREEN, my.LEDCOLOR_CYAN, my.LEDCOLOR_YELLOW, my.LEDCOLOR_WHITE };
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(50) RGB LED制御\n");
		M5.Log.printf("\tAtomS3Uなので RGB LED制御を行います。\n");
#endif	//DEBUG_MODE
		// 黒、青、赤、紫、緑、水、黄、白の順に 100msずつ点灯
		for(int ii = 0; ii < (sizeof(colorTable) / sizeof(colorTable[0])); ii++)
		{
			setRGBLED(colorTable[ii]);						// 指定色で点灯して
			delay(300);										// 300ms待つ
		}
		setRGBLED(my.LEDCOLOR_INITIALIZING);				// 「初期化中」色
	}
#endif	//defined(ARDUINO_M5STACK_ATOMS3)

#if defined(__M5GFX_M5UNITMINIOLED__)
	//--------------------------------------------------------------------------
	//	(60) サブ画面初期化
	//--------------------------------------------------------------------------
	//	72x40ピクセル モノクロ
	//	デフォルトフォント 6x8で 12桁5行表示
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(60) サブ画面初期化\n");
#endif	//DEBUG_MODE
		//*** コンストラクタ → setup()呼び出し
		pMiniOLED = new M5UnitMiniOLED(M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL(), MY_I2C_Device::FREQ);
		// 初期化・横向きに
		pMiniOLED->init();
		pMiniOLED->setRotation(1);
		pMiniOLED->setColorDepth(1);							// mono color
		pMiniOLED->setFont(&fonts::Font0);						// 6x8Picel
#ifdef DEBUG_MODE
		M5.Log.printf("\tMini OLED Unit initialize.\n");
		pMiniOLED->printf("* Mini OLED Unit initialize.\n");
#endif	//DEBUG_MODE
	}
#endif	//defined(__M5GFX_M5UNITMINIOLED__)

	//--------------------------------------------------------------------------
	//	(70) I2Cポート初期化
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(70) I2C初期化\n");
#endif	//DEBUG_MODE
#if defined(ARDUINO_M5STACK_ATOMS3)
		setRGBLED(my.LEDCOLOR_I2C_INITIALIZING);			// 「I2C初期化中」色
#endif	//defined(ARDUINO_M5STACK_ATOMS3)

#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		//*** (70-10) StickC系の場合、無条件に Ext.8Pを Ex_I2Cに割り当てる。
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: @HY2.0-4P(Port-A)へのデバイス接続有無にかかわらず、Ex_I2Cには Ext.8P(SDA = %d, SCL = %d)を割り当てます\n", my.nickName, my.PIN_EXT_SDA, my.PIN_EXT_SCL);
#endif	//DBUGE_MODE
			// Ex_I2Cの接続を @HY2.0-4Pから Ext.8Pに切り替え
			// 既存の Ex_I2Cの接続を解除してから、Ext.8P(HAT)に Ex_I2Cを接続する
			i2c_port_t port = M5.Ex_I2C.getPort();
			bool bRet1 = M5.Ex_I2C.stop();
			bool bRet2 = M5.Ex_I2C.release();
			M5.Ex_I2C.setPort(port, my.PIN_EXT_SDA, my.PIN_EXT_SCL);
//			bool bRet3 = M5.Ex_I2C.begin(port, my.PIN_EXT_SDA, my.PIN_EXT_SCL);
#ifdef DEBUG_MODE
//			M5.Log.printf("\tM5.Ex_I2C.getPort() = %d, M5.Ex_I2C.stop() = %d, M5.Ex_I2C.release() = %d, M5.Ex_I2C.begin() = %d\n", port, bRet1, bRet2, bRet3);
			M5.Log.printf("\tM5.Ex_I2C.getPort() = %d, M5.Ex_I2C.stop() = %d, M5.Ex_I2C.release() = %d\n", port, bRet1, bRet2);
#endif	//DBUGE_MODE
			// begin(後、おまじないの 10ms Wait
//			delay(10);
		}
#endif	//defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)

		bRet = M5.In_I2C.begin();							// Ex_I2Cに合わせて明示的に begin()する
		bRet2 = M5.Ex_I2C.begin();							// M5Unifed v0.1.7以降、明示的に begin()しないと外部I2C有効にならないっぽい・・・
#ifdef DEBUG_MODE
		M5.Log.printf("\tIn_I2C: Enabled = %d, Port = %d, SDA = %d, SCL = %d, begin() = %d\n", M5.In_I2C.isEnabled(), M5.In_I2C.getPort(), M5.In_I2C.getSDA(), M5.In_I2C.getSCL(), bRet);
		M5.Log.printf("\tEx_I2C: Enabled = %d, Port = %d, SDA = %d, SCL = %d, begin() = %d\n", M5.Ex_I2C.isEnabled(), M5.Ex_I2C.getPort(), M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL(), bRet2);
#endif	//DEBUG_MODE

		//*** (90-10) 内部 I2Cスキャン
		{
			memset(my.bIn_I2C, 0x00, sizeof(my.bIn_I2C));
			M5.In_I2C.scanID(my.bIn_I2C);
#ifdef DEBUG_MODE
			M5.Log.printf("\t-------------------------------------------------------\n");
			M5.Log.printf("\tIn_I2C:  0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
			M5.Log.printf("\t------:---------------------------------");
			for(uint8_t addr = 0; addr < 0x80; addr++)
			{
				if((addr % 16) == 0)
				{
					M5.Log.printf("\n\t 0x%02X : ", addr);
				}
				M5.Log.printf(" %c", (my.bIn_I2C[addr] ? '*' : '.'));
			}
			M5.Log.printf("\n");
#endif	//DEBUG_MODE
		}

		//*** (90-20) 外部 I2Cスキャン
		{
			memset(my.bEx_I2C, 0x00, sizeof(my.bEx_I2C));
			M5.Ex_I2C.scanID(my.bEx_I2C);
#ifdef DEBUG_MODE
			M5.Log.printf("\t-------------------------------------------------------\n");
			M5.Log.printf("\tEx_I2C:  0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
			M5.Log.printf("\t------:---------------------------------");
			for(uint8_t addr = 0; addr < 0x80; addr++)
			{
				if((addr % 16) == 0)
				{
					M5.Log.printf("\n\t 0x%02X : ", addr);
				}
				M5.Log.printf(" %c", (my.bEx_I2C[addr] ? '*' : '.'));
			}
			M5.Log.printf("\n");
#endif	//DEBUG_MODE
		}
	}

#if defined(USE_USB_OTG)
	//--------------------------------------------------------------------------
	//	(80) USB初期化
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(80) USB初期化\n");
#endif	//DEBUG_MODE
#if defined(ARDUINO_M5STACK_ATOMS3)
		setRGBLED(my.LEDCOLOR_USB_INITIALIZING);			// 「USB初期化中」色
#endif	//defined(ARDUINO_M5STACK_ATOMS3)
		bRet = USB.begin();
		consumerControl.begin();
#ifdef DEBUG_MODE
		M5.Log.printf("\tUSB.begin() = %d\n", bRet);
#if defined(__M5GFX_M5UNITMINIOLED__)
		pMiniOLED->printf("USB.begin() = %d\n", bRet);
#endif	//defined(__M5GFX_M5UNITMINIOLED__)
#endif	//DEBUG_MODE

		//--------------------------------------------------------------------------
		//	起動時に1回ボリュームUp/Downして USBホストに接続したことを知らしめる
		//--------------------------------------------------------------------------
		// 音量アップ
		usbHidKeyboardPress(CONSUMER_CONTROL_VOLUME_INCREMENT);
		// ちょっと待つ
		delay(1000);
		// 音量ダウン
		usbHidKeyboardPress(CONSUMER_CONTROL_VOLUME_DECREMENT);
#ifdef DEBUG_MODE
		M5.Log.printf("\tVolume Up/Down for TEST\n");
		Serial2.printf("\tVolume Up/Down for TEST\n");
#if defined(ARDUINO_M5STACK_ATOMS3)							// for AtomS3/AtomS3U
		M5.Log.printf("- RGB LEDは緑のはず\n");
		AtomS3.dis.drawpix(my.LEDCOLOR_HI_GREEN);			// 緑
		AtomS3.dis.show();
		delay(1000);
#endif	//defined(ARDUINO_M5STACK_ATOMS3)
#endif	//DBUGE_MODE
	}
#endif	//defined(USE_USB_OTG)

#if false
	//--------------------------------------------------------------------------
	//	(90) スピーカー音量設定(setVolume()を実行するとリブートしてしまうので無効にしている)
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("(90) スピーカー初期化\n");
#endif	//DEBUG_MODE
		uint8_t vol = M5.Speaker.getVolume();
#ifdef DEBUG_MODE
		M5.Log.printf("- Volude = %s\n", vol);
		M5.Speaker.setVolume(16);
#endif	//DEBUG_MODE
	}
#endif

	//--------------------------------------------------------------------------
	//	(100) WiFi接続、NTPから現在日時取得、RTC設定
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(100) WiFi接続\n");
#endif	//DEBUG_MODE
#if defined(ARDUINO_M5STACK_ATOMS3)
		setRGBLED(my.LEDCOLOR_WIFI_INITIALIZING);			// 「WiFi初期化中」色
#endif	//defined(ARDUINO_M5STACK_ATOMS3)
		my.bWiFi = getNtpDateTime();

		//*** WiFi初期化終了時の時計再表示
		drawClock(true);
	}

	//--------------------------------------------------------------------------
	//	(110) デュアルボタンユニット初期化
	//--------------------------------------------------------------------------
#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(110) デュアルボタンユニット初期化\n");
#endif	//DBUGE_MODE
		bool bRet = db_begin(my.PIN_BLUE_BUTTON, my.PIN_RED_BUTTON);
#ifdef DEBUG_MODE
		M5.Log.printf("\t初期化 = %s\n", (bRet ? "成功" : "失敗"));
#endif	//DBUGE_MODE
	}
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)

	//--------------------------------------------------------------------------
	//	(120) I2Cデバイス初期化
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(120) I2Cデバイス初期化\n");
#endif	//DBUGE_MODE
#if defined(ARDUINO_M5STACK_ATOMS3)
		setRGBLED(my.LEDCOLOR_I2C_INITIALIZING);			// 「I2C初期化中」色
#endif	//defined(ARDUINO_M5STACK_ATOMS3)
		//*** (100-10) 内部I2Cデバイス初期化
		{
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
			//*** HMIモジュールチェック & 初期化 for Core
//			my.oHmiModule = new MY_I2C_Device(my.I2C_HMI_MODULE, &(M5.In_I2C), "HMI-Module");
			my.oHmiModule = new MY_I2C_HMI_MODULE(&(M5.In_I2C));
			bRet = (my.oHmiModule)->begin();
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: %s\n", (my.oHmiModule)->getName(), ((my.oHmiModule)->isEnabled() ? "有効" : "無効"));
#endif	//DBUGE_MODE
			mode = procHmiModule(my.oHmiModule);

			//*** エンコーダーモジュールチェック & 初期化 for Core
//			my.oEncoderModule = new MY_I2C_Device(my.I2C_ENCODER_MODULE, &(M5.In_I2C), "Encoder-Module");
			my.oEncoderModule = new MY_I2C_ENCODER_MODULE(&(M5.In_I2C));
			bRet = (my.oEncoderModule)->begin();
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: %s\n", (my.oEncoderModule)->getName(), ((my.oEncoderModule)->isEnabled() ? "有効" : "無効"));
#endif	//DBUGE_MODE
			mode = procEncoderModule(my.oEncoderModule);
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
		}

		//*** (100-20) 外部I2Cデバイス初期化
		{
			//--------------------------------------------------------------------------
			//	(100-20-10) まずは機種依存デバイス
			//--------------------------------------------------------------------------
			//*** エンコーダーHATチェック & 初期化 for StickC
			my.oEncoderHat = new MY_I2C_ENCODER_HAT(&(M5.Ex_I2C));
#ifdef DEBUG_MODE
			M5.Log.printf("\tエンコーダーHAT: 周波数 = %ld, アドレス = 0x%02X, 名前 = [%s]\n",
				(my.oEncoderHat)->getFreq(), (my.oEncoderHat)->getAddr(), (my.oEncoderHat)->getName());
#endif	//DBUGE_MODE
			bRet = (my.oEncoderHat)->begin();
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: %s\n", (my.oEncoderHat)->getName(), ((my.oEncoderHat)->isEnabled() ? "有効" : "無効"));
#endif	//DBUGE_MODE
			mode = procEncoderHat(my.oEncoderHat);

			//*** Mini JoyC HATチェック & 初期化 for StickC
			my.oMiniJoycHat = new MY_I2C_Device(my.I2C_MINIJOYC_HAT, &(M5.Ex_I2C), "MiniJoyC-HAT");
			bRet = (my.oMiniJoycHat)->begin();
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: %s\n", (my.oMiniJoycHat)->getName(), ((my.oMiniJoycHat)->isEnabled() ? "有効" : "無効"));
#endif	//DBUGE_MODE
			mode = procMiniJoycHat(my.oMiniJoycHat);

			//--------------------------------------------------------------------------
			//	(100-20-20) 次に共通デバイス
			//--------------------------------------------------------------------------
			//*** エンコーダーユニットチェック & 初期化 for ALL
			my.oEncoderUnit = new MY_I2C_ENCODER_UNIT(&(M5.Ex_I2C));
#ifdef DEBUG_MODE
			M5.Log.printf("\tエンコーダーユニット: 周波数 = %ld, アドレス = 0x%02X, 名前 = [%s]\n",
				(my.oEncoderUnit)->getFreq(), (my.oEncoderUnit)->getAddr(), (my.oEncoderUnit)->getName());
#endif	//DBUGE_MODE
			bRet = (my.oEncoderUnit)->begin();
//			bRet = (my.oEncoderUnit)->begin(M5.Ex_I2C.getPort());
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: %s\n", (my.oEncoderUnit)->getName(), ((my.oEncoderUnit)->isEnabled() ? "有効" : "無効"));
			if((my.oEncoderUnit)->isEnabled())
			{
				uint8_t ver = (my.oEncoderUnit)->getVer();
			}
#endif	//DBUGE_MODE
			mode = procEncoderUnit(my.oEncoderUnit);

			//*** ENV.Ⅲユニットチェック(温湿度) & 初期化 for ALL
			my.oEnvUnit_S = new MY_I2C_ENV_UNIT_S(&(M5.Ex_I2C));
#ifdef DEBUG_MODE
			M5.Log.printf("\tENV.Ⅲユニット(温湿度): 周波数 = %ld, アドレス = 0x%02X, 名前 = [%s]\n",
				(my.oEnvUnit_S)->getFreq(), (my.oEnvUnit_S)->getAddr(), (my.oEnvUnit_S)->getName());
#endif	//DBUGE_MODE
			bRet = (my.oEnvUnit_S)->begin();
//			bRet = (my.oEncoderUnit)->begin(M5.Ex_I2C.getPort());
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: %s\n", (my.oEnvUnit_S)->getName(), ((my.oEnvUnit_S)->isEnabled() ? "有効" : "無効"));
#endif	//DBUGE_MODE

#if false
			//*** ENV.Ⅲユニットチェック(気圧) & 初期化 for ALL
			my.oEnvUnit_Q = new MY_I2C_ENV_UNIT_Q(&(M5.Ex_I2C));
#ifdef DEBUG_MODE
			M5.Log.printf("\tENV.Ⅲユニット(気圧): 周波数 = %ld, アドレス = 0x%02X, 名前 = [%s]\n",
				(my.oEnvUnit_Q)->getFreq(), (my.oEnvUnit_Q)->getAddr(), (my.oEnvUnit_Q)->getName());
#endif	//DBUGE_MODE
			bRet = (my.oEnvUnit_Q)->begin();
//			bRet = (my.oEncoderUnit)->begin(M5.Ex_I2C.getPort());
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s: %s\n", (my.oEnvUnit_Q)->getName(), ((my.oEnvUnit_Q)->isEnabled() ? "有効" : "無効"));
#endif	//DBUGE_MODE
#endif
		}
	}

	//--------------------------------------------------------------------------
	//	(130) M5Dialのエンコーダー初期化
	//--------------------------------------------------------------------------
	{
		//*** エンコーダーチェック & 初期化 for M5Dial
		// 専用クラスで管理されているので、I2Cデバイスとしては管理しない
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(130) M5Dialエンコーダー初期化\n");
#endif	//DBUGE_MODE
		mode = procM5DialEncoder();
	}
	//*** RGB LED初期化
	mode = procMediaControl(MY::MEDIA_CONTROL::NORMAL, "setup");

	//--------------------------------------------------------------------------
	//	(140) ESP-NOW初期化
	//--------------------------------------------------------------------------
	{
#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("(140) ESP-NOW初期化\n");
#endif	//DBUGE_MODE
#if defined(ARDUINO_M5STACK_ATOMS3)
		setRGBLED(my.LEDCOLOR_ESP_INITIALIZING);			// 「ESP初期化中」色
#endif	//defined(ARDUINO_M5STACK_ATOMS3)
		my.bEspNow = MyESPNOW::init();
#ifdef DEBUG_MODE
		M5.Log.printf("\tMyESPNOW::init() = %d\n", my.bEspNow);
#endif	//DEBUG_MODE

		//*** 自身のMACアドレス取得
		memset(my.macAddr, 0x00, sizeof(my.macAddr));
		MyESPNOW::getMac(my.macAddr);
#ifdef DEBUG_MODE
		M5.Log.printf("\t自身の MAC-Address = %02X:%02X:%02X:%02X:%02X:%02X\n", my.macAddr[0], my.macAddr[1], my.macAddr[2], my.macAddr[3], my.macAddr[4], my.macAddr[5]);
#endif	//DEBUG_MODE
		//*** ESP-NOW Slaveチェック処理
		bRet = checkSlave();
#ifdef DEBUG_MODE
		M5.Log.printf("\tESP-NOW Slaveチェック処理 = %d\n", bRet);
#endif	//DBUGE_MODE
	}

#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_ATOMS3)
	//------------------------------------------------------
	//	ログ出力用 Serial2初期化
	//------------------------------------------------------
	Serial2.begin(my.SERIAL2_SPEED, my.SERIAL2_SETTING, my.PIN_EXT_RX2, my.PIN_EXT_TX2);
#endif

#ifdef DEBUG_MODE
		M5.Log.printf("--------------------------------------------------------------------------------\n");
		M5.Log.printf("表示エリア情報\n");
		M5.Log.printf("\t* area1(%d, %d)-(%d, %d) *\n", my.ar1.iXstt, my.ar1.iYstt, my.ar1.iXend, my.ar1.iYend);
		M5.Log.printf("\t* area2(%d, %d)-(%d, %d) *\n", my.ar2.iXstt, my.ar2.iYstt, my.ar2.iXend, my.ar2.iYend);
		M5.Log.printf("\t* area3(%d, %d)-(%d, %d) *\n", my.ar3.iXstt, my.ar3.iYstt, my.ar3.iXend, my.ar3.iYend);
#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		M5.Log.printf("\t* area4(%d, %d)-(%d, %d) *\n", my.ar4.iXstt, my.ar4.iYstt, my.ar4.iXend, my.ar4.iYend);
#endif	//DEBUG_MODE

#ifdef DEBUG_MODE
	M5.Log.printf("*******************************************************\n");
	M5.Log.printf("- setup() : end\n");
	M5.Log.printf("*******************************************************\n");
#endif	//DEBUG_MODE

#if defined(ARDUINO_M5STACK_ATOMS3)
	setRGBLED(my.LEDCOLOR_OFF);							// RGB LED消灯
#endif	//defined(ARDUINO_M5STACK_ATOMS3)

	return;
}

//**************************************************************************************************
//	メインループ
//**************************************************************************************************
void loop(void)
{
	static MY::MEDIA_CONTROL oldMode = MY::MEDIA_CONTROL::NORMAL;	// 前回メディアコントロールモード
	static uint32_t oldModeChangeTime = 0;							// 前回メディアコントロールモード変更時刻
	static bool oldModeRecvF = false;								// 前回メディアコントロールモード変更が受信データによるものか否か
	static uint32_t oldExecTime = 0;								// 前回 update()実行時刻
	static uint32_t cntCalled = 0;									// loop()が呼び出された回数
	static bool bWatchDogOld = true;
	static bool bWatchDog = true;
	MY::MEDIA_CONTROL nowMode;
	MY::MEDIA_CONTROL newMode = MY::MEDIA_CONTROL::NONE;
	const char* eventName = "loop";
	bool bRet;

#ifdef DEBUG_MODE
	//*** loop()が初めて呼び出された時のログ出力
	if(cntCalled++ == 0)
	{
		M5.Log.printf("*******************************************************\n");
		M5.Log.printf("- loop() : 初めて呼び出されました\n");
		M5.Log.printf("*******************************************************\n");
	}
#endif	//DEBUG_MODE
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_ATOMS3)
	//*** ATOM S3で UART2からのデータ受信
	{
		if(Serial2.available())
		{
			char buf[256];
			memset(buf, 0x00, sizeof(buf));
			int ii = 0;
			while (Serial2.available() > 0)
			{
				buf[ii++] = Serial2.read();
			}
			M5.Log.printf("--- from Serial2[%s]\n", buf);
		}
	}
#endif

	//*** 生存確認用 Watch Dog 100msごと LED点灯
	{
		static uint32_t oldExecMillis = 0;					// 前回実行時刻
		const int brightMax = 250;							// RGB LED最大輝度
		static int bright = brightMax;						// RGB LED現在輝度：最大輝度から始める

		int colorRGB;

		// 前回実行時刻より 100ms経過していたら
		if((oldExecMillis + 150) < millis())
		{
			// 前回実行時刻更新
			oldExecMillis = millis();
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
#if defined(__EspEasyLED_H__)
			//*** Core系のバッテリーボトムの場合
			const int briPtnCnt = 3;						// 輝度パターン数
			const int briTblCnt = 5;						// 輝度テーブル数
			// シリアル RGB LEDのカラーコードテーブル：右上 → 右下 → 左下 → 左上(0時から時計回り)
			// 順に R: 赤, G: 緑, B: 青の輝度
#if false
			// 下から上に流れていくイメージ
			static uint8_t briTblUP[briTblCnt][3] = {
				{  50,  50,  50 },
				{ 100, 100, 100 },
				{ 150, 150, 150 },
				{ 200, 200, 200 },
				{   0, 250,   0 },							// ここだけ緑
			};
			// 上から下に流れていくイメージ
			static uint8_t briTblDOWN[briTblCnt][3] = {
				{ 250,   0,   0 },							// ここだけ赤
				{ 200, 200, 200 },
				{ 150, 150, 150 },
				{ 100, 100, 100 },
				{  50,  50,  50 },
			};
#endif
			// 輝度パターン
			static uint8_t briTbl[briPtnCnt][briTblCnt][3] = {
				{											// 下から上に緑が流れていくパターン
					{  50,  50,  50 },
					{ 100, 100, 100 },
					{ 150, 150, 150 },
					{ 200, 200, 0 },
					{   0, 250,   0 },							// ここだけ緑
				},
				{
					{ 250,   0,   0 },							// ここだけ赤
					{ 200, 200, 200 },
					{ 150, 150, 150 },
					{ 100, 100, 100 },
					{  50,  50,  50 },
				},
				{
					{   0,   0, 250 },							// ここだけ青
					{ 200, 200, 200 },
					{ 150, 150, 150 },
					{ 100, 100, 100 },
					{  50,  50,  50 },
				},
			};
			static int briPtnNo = 0;					// 輝度パターンの何番目を使用するか
			static int briTblNo = 0;					// 輝度テーブル内のインデックス

			if(!bWatchDog)
			{
				if(bWatchDogOld)
				{
					briTblNo = 0;
					briPtnNo++;
					if((sizeof(briTbl)/sizeof(briTbl[0])) <= briPtnNo)
					{
						briPtnNo = 0;
					}
#ifdef DEBUG_MODE
					M5.Log.printf("\tNext BriPtnNo = %d\n", briPtnNo);
#endif	//DEBUG_MODE
				}
				bWatchDogOld = false;
			}
			else
			{
				bWatchDogOld = true;
				// (1-1) コントロールモードが変更されて 3s未満なら、コントロールモード変更を示す色
				if(oldExecMillis < (oldModeChangeTime + 3000))
				{
					// (1) 基準となるカラーコード(輝度情報含む)を算出
					switch(oldMode)
					{
						// 前のトラック
						case MY::MEDIA_CONTROL::PREVIOUS:
//							colorRGB = my.LEDCOLOR_MAGENTA;
//							colorRGB = 0x7F007F;
							colorRGB = ((bright << 16) | bright);
							break;
						// 次のトラック
						case MY::MEDIA_CONTROL::NEXT:
//							colorRGB = my.LEDCOLOR_CYAN;
//							colorRGB = 0x007F7F;
							colorRGB = ((bright << 8) | bright);
							break;
						// 一時停止
						case MY::MEDIA_CONTROL::PAUSE:
//							colorRGB = my.LEDCOLOR_YELLOW;
//							colorRGB = 0x7F7F00;
							colorRGB = ((bright << 16) | (bright << 8));
							break;
						// 音量ダウン
						case MY::MEDIA_CONTROL::VDOWN:
//							colorRGB = my.LEDCOLOR_GREEN;
//							colorRGB = 0x007F00;
							colorRGB = (bright << 8);
							break;
						// 音量アップ
						case MY::MEDIA_CONTROL::VUP:
//							colorRGB = my.LEDCOLOR_BLUE;
//							colorRGB = 0x00007F;
							colorRGB = bright;
							break;
						// ミュート
						case MY::MEDIA_CONTROL::MUTE:
//							colorRGB = my.LEDCOLOR_RED;
//							colorRGB = 0x7F0000;
							colorRGB = (bright << 16);
							break;
						// その他
						default:
//							colorRGB = my.LEDCOLOR_WHITE;
//							colorRGB = 0x7F7F7F;
							colorRGB = ((bright << 16) | (bright << 8) | bright);
							break;
					}
				}
				// (1-2) 3s経っていたら、待機中を示す色
				else
				{
//					colorRGB = my.LEDCOLOR_WHITE;
//					colorRGB = 0x7F7F7F;
					colorRGB = ((bright << 16) | (bright << 8) | bright);
					oldMode = MY::MEDIA_CONTROL::NORMAL;
				}

				// (2) 明るさ上限を設定
				led.setBrightness(50);

				// (3) 右側5個のシリアル RGB LED：右上 → 右下、左側5個のシリアル RGB LED：左上 → 左下の順に同時に同じ値を設定
				for(int idx = 0; idx <= briTblCnt; idx++)
				{
					// ループは(10 / 2) + 1 = 6回だが、カラーコードを設定するのは最初の 5回のみ
					// 最後は次の WatchDog用
					if(idx < briTblCnt)
					{
#if true
						led.setColor(idx, briTbl[briPtnNo][briTblNo][0], briTbl[briPtnNo][briTblNo][1], briTbl[briPtnNo][briTblNo][2]);							// 右側
						led.setColor(((briTblCnt * 2  - 1) - idx), briTbl[briPtnNo][briTblNo][0], briTbl[briPtnNo][briTblNo][1], briTbl[briPtnNo][briTblNo][2]);	// 左側
#else
						led.setColor(idx, briTblUP[briTblNo][0], briTblUP[briTblNo][1], briTblUP[briTblNo][2]);							// 右側
						led.setColor(((briTblCnt * 2  - 1) - idx), briTblUP[briTblNo][0], briTblUP[briTblNo][1], briTblUP[briTblNo][2]);	// 左側
#endif
					}
					// 次のカラーインデックステーブルのインデックス値がテーブル数内に収まるように調整
					briTblNo++;
					if(briTblCnt <= briTblNo)
					{
						briTblNo = 0;
					}
				}
				// (4) RGB LED点灯
				led.show();
			}
#endif	//defined(__EspEasyLED_H__)
#elif defined(ARDUINO_M5STACK_ATOMS3)
			//*** AtomS3Uの RGB LEDの場合
			{
				//*** RGB LEDのカラーコード(明るさ含む)設定
				//*** ESPモードと現在輝度よりカラーコード算出：RGB = (Red * 0x10000) + (Green * 0x100) + Blue)
				switch(my.espMode)
				{
					// SLAVE：黄
					case MY::ESP_MODE::SLAVE:
						// コントロールモードが変更されて 3s未満なら、コントロールモード変更を示す色(青or緑)
						if(oldExecMillis < (oldModeChangeTime + 3000))
						{
							// 受信データによるモード変更：青は暗いので2倍の輝度
							if(oldModeRecvF)
							{
								colorRGB = (bright << 1);
							}
							// 自身のデバイスによる変更：緑
							else
							{
								colorRGB = (bright << 8);
							}
						}
						// そうでなければ待機中の色(黄)
						else
						{
							colorRGB = ((bright << 16) | (bright << 8) | 0x00);
							oldMode = MY::MEDIA_CONTROL::NORMAL;
						}
						break;
					// CONTROLLER：水
					case MY::ESP_MODE::CONTROLLER:
						colorRGB = ((bright << 8) | bright);
						break;
					// 未設定・その他：紫
					case MY::ESP_MODE::NO_INIT:
					default:
						colorRGB = ((bright << 16) | bright);
						break;
				}
				// RGB LEDセット
#if defined(ARDUINO_M5STACK_ATOMS3)
				setRGBLED(colorRGB);
#endif	//defined(ARDUINO_M5STACK_ATOMS3)
			}
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
		}
	}

	//*** WatchDog(10s)
#ifdef DEBUG_MODE
	//*** 生存確認用 Watch Dog 10秒ごとにログ出力
	{
		static uint32_t oldExecMillis = 0;
		static uint32_t oldCntCalled = 0;					// 前回呼び出された時の回数
		if((oldExecMillis + 10000) < millis())
		{
			M5.Log.printf("***** loop() : watch dog(every 10,000ms) : 前回回数 = %'d, 今回回数 = %'d, 差 = %'d *****\n", oldCntCalled, cntCalled, (cntCalled - oldCntCalled));
			// 前回値として保存
			oldExecMillis = millis();
			oldCntCalled = cntCalled;						// 前回呼び出された時の回数
		}
	}
#endif	//DEBUG_MODE

	// ESPモード未設定時のタイムアウトなら、自身を Slaveに設定
	if((my.espMode == MY::ESP_MODE::NO_INIT) && ((my.requestMacTime + my.DELAY_ESPNOW_TIMEOUT) < millis()))
	{
#ifdef DEBUG_MODE
		M5.Log.printf("***********************************************************************************************************\n");
		M5.Log.printf(" REQUEST_MACコマンド送信への応答がタイムアウト(%d)したので、自身が Slaveになります。\n", my.DELAY_ESPNOW_TIMEOUT);
		M5.Log.printf("***********************************************************************************************************\n");
#endif	//DEBUG_MODE
		//------------------------------------------------------
		//	Slaveへの MACアドレス要求がタイムアウトしたので、自身が Slaveになる
		//------------------------------------------------------
		my.espMode = MY::ESP_MODE::SLAVE;
		// ESP-NOW Slave/Controllerモード表示
		drawEspMode(my.espMode);
	}

	//--------------------------------------------------------------------------
	//	update()を頻繁に呼び出さないように delay(100)相当のロジック
	//--------------------------------------------------------------------------
	//	Notice():
	//		update()を delay()なしで呼び出すとボタン状態を取りこぼすので、delay(100)相当のロジックを追加。
	//		ただし、M5Dialでは delay()するとエンコーダー値を取りこぼすので、delay()したくない。
	//		ただし、エンコーダーユニットの反応がセンシティブすぎるので、前回エンコーダーユニット処理を行っていたらdelay()。
	//--------------------------------------------------------------------------
	// 前回loop()実行時から100ms経過してなければ、何もしない
	if(millis() < (oldExecTime + my.DELAY_LOOP))
	{
		return;
	}
#if defined(ARDUINO_M5STACK_DIAL)
	// M5Dialではエンコーダーユニット処理を行った場合に、前回Loop()実行時の時間を保存
#else
	// M5Dial以外では毎回前回Loop()実行時の時間を保存
	oldExecTime = millis();
#endif	//defined(ARDUINO_M5STACK_DIAL)

	//--------------------------------------------------------------------------
	//	update()
	//--------------------------------------------------------------------------
#if defined(ARDUINO_M5STACK_ATOMS3)
	AtomS3.update();
#else
	M5.update();
#endif	//defined(ARDUINO_M5STACK_ATOMS3)

	//--------------------------------------------------------------------------
	//	画面描画
	//--------------------------------------------------------------------------
	//*** 描画開始
	M5.Display.startWrite();
	M5.Display.waitDisplay();

	//*** 時計表示
	drawClock(false);

#if defined(DISP_MODE_STICKC)	// StickC系場合
	//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
	//*** ユニットチェック用：ENV.Ⅲ
	//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
	if((my.oEnvUnit_S)->isEnabled())
	{
		float fValueT = (my.oEnvUnit_S)->getValue();	// ENV.Ⅲユニットから温度取得
		float fValueH = (my.oEnvUnit_S)->getValue2();	// ENV.Ⅲユニットから湿度取得
		M5.Display.setFont(&fonts::Font2);

		M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
		M5.Display.setCursor(0, 180);
		M5.Display.printf("* %3.2f degrees C   \n", fValueT);
		M5.Display.setTextColor(TFT_BLACK, TFT_CYAN);
		M5.Display.setCursor(0, 200);
		M5.Display.printf("* %3.2f rH   \n", fValueH);
	}
#endif	//defined(DISP_MODE_STICKC)	// StickC系場合
	//*** 描画終了
	M5.Display.display();
	M5.Display.endWrite();

#if false	// AtomS3Uで watchdogとして 100msごとに点灯させているから、ここでは不要に
	//*** RGB LED設定(待機状態)
	newMode = procMediaControl(MY::MEDIA_CONTROL::NORMAL, "loop");
#endif

	//--------------------------------------------------------------------------
	//	ESP-NOW受信データ処理
	//--------------------------------------------------------------------------
	if(MyESPNOW::isCalledRecv())			// データを受信していたら
	{
		MyESPNOW::setCalledRecv(false);		// データ受信フラグをリセット
		// ESP-NOW 受信データ処理。電文によっては内部でメディアコントロール処理を行っている。
		if((nowMode = procEspNowRecvData()) != MY::MEDIA_CONTROL::NONE)
		{
			newMode = nowMode;
			oldModeRecvF = true;
		}
	}



#if	defined(ARDUINO_M5STACK_DIAL)							// for Dial
	//--------------------------------------------------------------------------
	//	M5Dialの NFCチェック
	//--------------------------------------------------------------------------
	{
		if(M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial())
		{
			uint8_t piccType = M5Dial.Rfid.PICC_GetType(M5Dial.Rfid.uid.sak);
			// UIDを16進数文字列に変換
			String uidStr = "";
			for(byte i = 0; i < M5Dial.Rfid.uid.size; i++)
			{
				if(M5Dial.Rfid.uid.uidByte[i] < 0x10)
				{
					uidStr += "0";
				}
				uidStr += String(M5Dial.Rfid.uid.uidByte[i], HEX);
				if(i < M5Dial.Rfid.uid.size - 1)
				{
					uidStr += ":";
				}
			}
			uidStr.toUpperCase();
			// シリアル出力
			Serial.print("Card UID: ");
			Serial.println(uidStr);
			// 画面にUIDを表示
			M5Dial.Display.clear();
			M5Dial.Display.setTextColor(WHITE);
			M5Dial.Display.drawString("Card Detected!", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 20);
			M5Dial.Display.setTextColor(YELLOW);
			M5Dial.Display.drawString(uidStr, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 10);
			// 通信終了処理
			M5Dial.Rfid.PICC_HaltA();
//			delay(1000); // 連続読み込みの調整用ウェイト
			// 元表示に戻す
			M5Dial.Display.clear();
			M5Dial.Display.setTextColor(GREEN);
			M5Dial.Display.drawString("Touch Card", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
		}
	}
#endif defined(ARDUINO_M5STACK_DIAL)							// for Dial



#if defined(ARDUINO_M5STACK_TAB5)
	//*** Tab5ではボタンが使用できないのでボタン判定を行わない
#else
#if defined(ARDUINO_M5STACK_COREINK)
	//*** CORE.INKの特殊ボタン
	{
		//--------------------------------------------------------------------------
		//	ボタンEXT判定：上部ボタン(GPIO5)
		//--------------------------------------------------------------------------
		if((nowMode = procButton(MY::BUTTON::BUTTON_EXT, &(M5.BtnEXT), &eventName)) != MY::MEDIA_CONTROL::NONE)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\tBtn Ext@CORE.INK\n");
#endif	//DEBUG_MODE
			newMode = procMediaControl(nowMode, eventName);

#if false
#if defined(__EspEasyLED_H__)
			led.setBrightness(20);
			led.showColor(0, 255, 0);
#endif	//defined(__EspEasyLED_H__)
#endif

			Serial2.write("Button EXT wasClicked()\n");
		}

		//--------------------------------------------------------------------------
		//	ボタンPWR判定
		//--------------------------------------------------------------------------
		if(M5.BtnPWR.wasClicked())
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\tBtn PWR@CORE.INK\n");
#endif	//DEBUG_MODE
			M5.Log.printf("***********************************************************************************************************\n");
			M5.Log.printf("\tCORE.INK ShutDown...\n");
			M5.Log.printf("***********************************************************************************************************\n");
			M5.Power.powerOff();
		}
	}
#endif	//defined(ARDUINO_M5STACK_COREINK)
	//*** CORE.INK以外の機種の共通ボタンA、ボタンB、ボタンC判定
	{
		//--------------------------------------------------------------------------
		//	ボタンA判定
		//--------------------------------------------------------------------------
		if((nowMode = procButton(MY::BUTTON::BUTTON_A, &(M5.BtnA), &eventName)) != MY::MEDIA_CONTROL::NONE)
		{
			newMode = procMediaControl(nowMode, eventName);

#if false
#if defined(__EspEasyLED_H__)
			led.setBrightness(20);
			led.showColor(0, 255, 0);
#endif	//defined(__EspEasyLED_H__)
#endif

			Serial2.write("Button A wasClicked()\n");
		}

		//--------------------------------------------------------------------------
		//	ボタンB判定
		//--------------------------------------------------------------------------
		if((nowMode = procButton(MY::BUTTON::BUTTON_B, &(M5.BtnB), &eventName)) != MY::MEDIA_CONTROL::NONE)
		{
			newMode = procMediaControl(nowMode, eventName);

#if false
#if defined(__EspEasyLED_H__)
			led.setBrightness(20);
			led.showColor(0, 255, 0);
#endif	//defined(__EspEasyLED_H__)
#endif

			Serial2.write("Button B wasClicked()\n");
		}

		//--------------------------------------------------------------------------
		//	ボタンC判定
		//--------------------------------------------------------------------------
		if((nowMode = procButton(MY::BUTTON::BUTTON_C, &(M5.BtnC), &eventName)) != MY::MEDIA_CONTROL::NONE)
		{
			newMode = procMediaControl(nowMode, eventName);

#if false
#if defined(__EspEasyLED_H__)
			led.setBrightness(20);
			led.showColor(0, 255, 0);
#endif	//defined(__EspEasyLED_H__)
#endif

			Serial2.write("Button C wasClicked()\n");
		}
	}
#endif	//defined(ARDUINO_M5STACK_TAB5)

	//--------------------------------------------------------------------------
	//	タッチパネル処理 for Core2/CoreS3/M5Dial/Tab5
	//--------------------------------------------------------------------------
	if((nowMode = procTouchPanel(&eventName)) != MY::MEDIA_CONTROL::NONE)
	{
		newMode = procMediaControl(nowMode, eventName);
		oldModeRecvF = false;
		bWatchDog = !bWatchDog;
	}

	//--------------------------------------------------------------------------
	//	エンコーダー処理 for M5Dial
	//--------------------------------------------------------------------------
	if((nowMode = procM5DialEncoder()) != MY::MEDIA_CONTROL::NONE)
	{
		newMode = procMediaControl(nowMode, "M5Dial");
		oldModeRecvF = false;
	}

	//--------------------------------------------------------------------------
	//	エンコーダーモジュール処理 for Core
	//--------------------------------------------------------------------------
	if((nowMode = procEncoderModule(my.oEncoderModule)) != MY::MEDIA_CONTROL::NONE)
	{
		newMode = procMediaControl(nowMode, (my.oEncoderModule)->getName());
		oldModeRecvF = false;
	}

	//--------------------------------------------------------------------------
	//	HMIモジュール処理 for Core
	//--------------------------------------------------------------------------
	if((nowMode = procHmiModule(my.oHmiModule)) != MY::MEDIA_CONTROL::NONE)
	{
		newMode = procMediaControl(nowMode, (my.oHmiModule)->getName());
		oldModeRecvF = false;
	}

	//--------------------------------------------------------------------------
	//	エンコーダーユニット処理 for ALL
	//--------------------------------------------------------------------------
	if((nowMode = procEncoderUnit(my.oEncoderUnit)) != MY::MEDIA_CONTROL::NONE)
	{
		newMode = procMediaControl(nowMode, (my.oEncoderUnit)->getName());
		oldModeRecvF = false;

#if defined(ARDUINO_M5STACK_DIAL)
		// M5Dialではエンコーダーユニット処理を行った場合に、前回Loop()実行時の時間を保存
		oldExecTime = (bRet ? millis() : oldExecTime);
#endif	//defined(ARDUINO_M5STACK_DIAL)
	}

	//--------------------------------------------------------------------------
	//	エンコーダーHAT処理 for StickC
	//--------------------------------------------------------------------------
	if((nowMode = procEncoderHat(my.oEncoderHat)) != MY::MEDIA_CONTROL::NONE)
	{
		newMode = procMediaControl(nowMode, (my.oEncoderHat)->getName());
		oldModeRecvF = false;
	}

	//--------------------------------------------------------------------------
	//	Mini JoyC HAT処理 for StickC
	//--------------------------------------------------------------------------
	if((nowMode = procMiniJoycHat(my.oMiniJoycHat)) != MY::MEDIA_CONTROL::NONE)
	{
		newMode = procMediaControl(nowMode, (my.oMiniJoycHat)->getName());
		oldModeRecvF = false;
	}

	//*** メディアコントロールモードが変わったら前回値を保存
	if((newMode != MY::MEDIA_CONTROL::NONE) && (newMode != oldMode))
	{
#ifdef DEBUG_MODE
//		M5.Log.printf("\tメディアコントロールモードが変更されました(%d → %d)@%d\n", oldMode, newMode, millis());
#endif	//DEBUG_MODE
		oldMode = newMode;									// 前回コントロールモードとして保存
		oldModeChangeTime = millis();						// 前回コントロールモード変更時刻として保存
	}

	//********** delay()相当の処理を loop()先頭で行っている **********
	//delay(my.DELAY_LOOP);

	return;
}



//------------------------------------------------------------------------------
//	受信データ処理
//
//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
//	  ┃  ┗ ESP-NOW Slave/Controllerモード
//	  ┗ コマンドコード
//------------------------------------------------------------------------------
MY::MEDIA_CONTROL procEspNowRecvData(void)
{
	bool bRet = false;
	MY::MEDIA_CONTROL nowMode;

	// 受信データ
	uint8_t recvMAC[6 + 1];
	uint8_t recvData[250 + 1];
	int recvLen;
	MY::MEDIA_CONTROL newMode = MY::MEDIA_CONTROL::NONE;	// メディアコントロールモード
	const char* mediaControlModeName = NULL;				// メディアコントロールモード名
	// 送信データ
	esp_err_t espErr;
	uint8_t sendData[250 + 1];								// 送信可能250バイト +1
	int8_t sendLen = 0;										// 送信データ長

	int ii;
	int mode;

	//*** 受信データ取得：recvMAC[]に MACアドレス, recvData[]に受信データがそれぞれセットされる
	recvLen = MyESPNOW::recv(recvMAC, recvData);

	//*** 電文内のコマンドコード/ESP-NOW Slave/Controllerモード
	uint8_t* addr = recvMAC;
	uint8_t* data = recvData;
	MY::DEVICE_CONTROL ctrlMode = (MY::DEVICE_CONTROL)(*data++);	// (0)コマンドコード
	MY::ESP_MODE espMode = (MY::ESP_MODE)(*data++);					// (1)送信元の ESP-NOW Slave/Controllerモード

	int8_t recvPeerIdx = MyESPNOW::checkPeerRegisterd(recvMAC);		// 送信元MACアドレスのピアテーブル登録インデックスNoの取得
	uint8_t* recvName = MyESPNOW::getNickName(recvPeerIdx);			// インデックスよりニックネームの取得
	const char* modeName = getModeName(ctrlMode);					// コマンドコード名の取得

#ifdef DEBUG_MODE
	// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
	M5.Log.printf("*** procEspNowRecvData(): %s from [%s](Idx=%d, Mode=%d)[%02X:%02X:%02X:%02X:%02X:%02X] ***\n", 
		modeName, recvName, recvPeerIdx, espMode, *(recvMAC + 0), *(recvMAC + 1), *(recvMAC + 2), *(recvMAC + 3), *(recvMAC + 4), *(recvMAC + 5)
	);
#endif	//DEBUG_MODE

	// コマンドコード別処理
	switch(ctrlMode)
	{
		//--------------------------------------------------------------------------
		//	MACアドレス要求
		//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
		//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
		//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
		//	          ┗━━━━━━━━━┛：送信元MACアドレス
		//	※ この電文にはニックネームは含まれていません！
		//--------------------------------------------------------------------------
		case MY::DEVICE_CONTROL::REQUEST_MAC:
#ifdef DEBUG_MODE
			M5.Log.printf("\t電文内MACアドレス=[%02X:%02X:%02X:%02X:%02X:%02X]\n", *(data + 0), *(data + 1), *(data + 2), *(data + 3), *(data + 4), *(data + 5));
#endif	//DEBUG_MODE
			//*** 自身が Contorollerなら何もしない
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
#ifdef DEBUG_MODE
				M5.Log.printf("\t自身は Controllerなので MACアドレス要求には応答しません\n");
#endif	//DEBUG_MODE
				break;
			}
			//*** 送信元MACアドレスと電文内MACアドレスが異なる場合、何もしない
			if(memcmp(addr, data, 6))
			{
#ifdef DEBUG_MODE
				M5.Log.printf("\t送信元MACアドレスと電文内MACアドレスが異なるので応答しません\n");
#endif	//DEBUG_MODE
				break;
			}

			//*** 送信元のMACアドレスがピアテーブルに未登録なら
			if(recvPeerIdx == -1)
			{
				// 送信元のMACアドレスをピアテーブルに新規登録
				if((recvPeerIdx = MyESPNOW::registPeer(addr)) != -1)
				{
					;
				}
			}

			//*** 送信元に自身のMACアドレスを通知する
			if(recvPeerIdx != -1)
			{
				//*** 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_MAC;			// MACアドレス通知
				sendData[sendLen++] = my.espMode;							// Slave/Controllerモード
				memcpy(&sendData[sendLen], my.macAddr, sizeof(my.macAddr));	// 自身のMACアドレス
				sendLen += 6;
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(recvPeerIdx, sendData, (sendLen + 1));
			}

			break;

		//--------------------------------------------------------------------------
		//	MACアドレス通知
		//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
		//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
		//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
		//	          ┗━━━━━━━━━┛：送信元MACアドレス
		//--------------------------------------------------------------------------
		case MY::DEVICE_CONTROL::SEND_MAC:
#ifdef DEBUG_MODE
			// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
			M5.Log.printf("\t電文内MACアドレス=[%02X:%02X:%02X:%02X:%02X:%02X]\n", *(data + 0), *(data + 1), *(data + 2), *(data + 3), *(data + 4), *(data + 5));
#endif	//DEBUG_MODE

			//*** 送信元MACアドレスと電文内MACアドレスが異なる場合、何もしない
			if(memcmp(addr, data, 6))
			{
#ifdef DEBUG_MODE
				M5.Log.printf("\t送信元MACアドレスと電文内MACアドレスが異なるので応答しません\n");
#endif	//DEBUG_MODE
				break;
			}

			//*** SlaveからのMACアドレス通知の場合
			if(espMode == MY::ESP_MODE::SLAVE)
			{
				// REQUEST_MAC送信への応答がタイムアウト時間内なら、自身は Contorollerになる
				if(millis() < (my.requestMacTime + my.DELAY_ESPNOW_TIMEOUT))
				{
#ifdef DEBUG_MODE
					M5.Log.printf("***********************************************************************************************************\n");
					M5.Log.printf(" REQUEST_MACコマンド送信への応答が %dms内にあったので、自身は Controllerになります。\n", my.DELAY_ESPNOW_TIMEOUT);
					M5.Log.printf("***********************************************************************************************************\n");
#endif	//DEBUG_MODE
					my.espMode = MY::ESP_MODE::CONTROLLER;
					// ESP-NOW Slave/Controllerモード表示
					drawEspMode(my.espMode);
				}
			}

			//*** 送信元のMACアドレスがピアテーブルに未登録なら
			if(recvPeerIdx == -1)
			{
				// 送信元のMACアドレスをピアテーブルに新規登録、成功したらニックネーム要求
				if((recvPeerIdx = MyESPNOW::registPeer(addr)) != -1)
				{
					//*** 送信データ作成
					memset(sendData, 0x00, sizeof(sendData));
					sendLen = 0;
					sendData[sendLen++] = MY::DEVICE_CONTROL::REQUEST_NAME;			// ニックネーム要求
					sendData[sendLen++] = my.espMode;								// Slave/Controllerモード
					memcpy(&sendData[sendLen], my.nickName, strlen(my.nickName));	// 自身のニックネーム
					sendLen += strlen(my.nickName);
#ifdef DEBUG_MODE
					// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
					M5.Log.printf("------------------------------------------------------------------------------------------------------------\n");
					M5.Log.printf("(S) 通知元に対してニックネーム要求 my.nickName = [%s] to [%s](Idx=%d)[%02X:%02X:%02X:%02X:%02X:%02X]\n",
						my.nickName,
						MyESPNOW::getNickName(recvPeerIdx), recvPeerIdx,
						*(addr + 0), *(addr + 1), *(addr + 2), *(addr + 3), *(addr + 4), *(addr + 5)
					);
					M5.Log.printf("------------------------------------------------------------------------------------------------------------\n");
#endif	//DEBUG_MODE
					//*** ESP-NOW送信処理
					bRet = MyESPNOW::send(recvPeerIdx, sendData, (sendLen + 1));
				}
			}

			//*** SlaveからのMACアドレス通知の場合
			if(espMode == MY::ESP_MODE::SLAVE)
			{
				// SlaveのピアIndex
				my.slaveIdx = recvPeerIdx;
			}

			break;

		//--------------------------------------------------------------------------
		//	ニックネーム要求
		//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
		//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
		//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
		//	          ┗━━━━━━━━━～：要求元のニックネーム
		//--------------------------------------------------------------------------
		case MY::DEVICE_CONTROL::REQUEST_NAME:
#ifdef DEBUG_MODE
			// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
			M5.Log.printf("\t電文内ニックネーム = [%s]\n", data);
#endif	//DEBUG_MODE

			//*** 送信元のMACアドレスがピアテーブルに未登録なら
			if(recvPeerIdx == -1)
			{
				// 送信元のMACアドレスをピアテーブルに新規登録
				if((recvPeerIdx = MyESPNOW::registPeer(addr)) != -1)
				{
					;
				}
			}

			//*** 送信元に自身のニックネームを通知する
			if(recvPeerIdx != -1)
			{
				// ニックネーム登録
				bRet = MyESPNOW::registNickName(recvPeerIdx, data);
				//*** 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_NAME;			// ニックネーム通知
				sendData[sendLen++] = my.espMode;								// Slave/Controllerモード
				memcpy(&sendData[sendLen], my.nickName, strlen(my.nickName));	// 自身のニックネーム
				sendLen += strlen(my.nickName);
#ifdef DEBUG_MODE
				// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
				M5.Log.printf("------------------------------------------------------------------------------------------------------------\n");
				M5.Log.printf("(S) 要求元に対してニックネーム通知 data = [%s] to [%s](Idx=%d)[%02X:%02X:%02X:%02X:%02X:%02X]\n",
					my.nickName, recvName, recvPeerIdx,
					*(addr + 0), *(addr + 1), *(addr + 2), *(addr + 3), *(addr + 4), *(addr + 5)
				);
#endif	//DEBUG_MODE
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(recvPeerIdx, sendData, (sendLen + 1));
			}

			break;

		//--------------------------------------------------------------------------
		//	ニックネーム通知
		//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
		//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
		//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
		//	          ┗━━━━━━━━━～：ニックネーム
		//--------------------------------------------------------------------------
		case MY::DEVICE_CONTROL::SEND_NAME:
#ifdef DEBUG_MODE
			// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
			M5.Log.printf("\t電文内ニックネーム = [%s]\n", data);
#endif	//DEBUG_MODE

			//*** 送信元のMACアドレスがピアテーブルに未登録なら
			if(recvPeerIdx == -1)
			{
				// 送信元のMACアドレスをピアテーブルに新規登録
				if((recvPeerIdx = MyESPNOW::registPeer(addr)) != -1)
				{
					;
				}
			}
			// ニックネーム登録
			bRet = MyESPNOW::registNickName(recvPeerIdx, data);

			break;

		//--------------------------------------------------------------------------
		//	キーコード送信要求
		//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
		//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
		//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
		//	          ┗ メディアコントロールモード
		//--------------------------------------------------------------------------
		case MY::DEVICE_CONTROL::SEND_KEYCODE:
			//*** メディアコントロールモード取得
			nowMode = (MY::MEDIA_CONTROL)(*data++);
			mediaControlModeName = getMediaControlModeName(nowMode);	// メディアコントロールモード名の取得
#ifdef DEBUG_MODE
			// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
			M5.Log.printf("\tメディアコントロールモード = 0x%02X[%s]\n", nowMode, mediaControlModeName);
#endif	//DEBUG_MODE

			//*** 自身が Contorollerなら何もしない
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
#ifdef DEBUG_MODE
				M5.Log.printf("\t自身は Controllerなので キーコード送信要求には応答しません\n");
#endif	//DEBUG_MODE
				break;
			}
#if false
			//*** 送信元MACアドレスと電文内MACアドレスが異なる場合、何もしない
			if(memcmp(addr, data, 6))
			{
#ifdef DEBUG_MODE
				M5.Log.printf("\t送信元MACアドレスと電文内MACアドレスが異なるので応答しません\n");
#endif	//DEBUG_MODE
				break;
			}
#endif

			//*** 送信元のMACアドレスがピアテーブルに未登録なら
			if(recvPeerIdx == -1)
			{
				// 送信元のMACアドレスをピアテーブルに新規登録
				if((recvPeerIdx = MyESPNOW::registPeer(recvMAC)) != -1)
				{
					;
				}
			}

			//*** SlaveからのMACアドレス通知の場合
			if(espMode == MY::ESP_MODE::SLAVE)
			{
				// SlaveのピアIndex
				my.slaveIdx = recvPeerIdx;
			}

			//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
			newMode = procMediaControl(nowMode, (const char*)recvName);
			//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■

			break;

		//--------------------------------------------------------------------------
		//	マウスコントロールコード送信要求
		//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
		//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
		//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
		//	          ┗ 
		//--------------------------------------------------------------------------
		case MY::DEVICE_CONTROL::SEND_MOUSE:
			break;

		//--------------------------------------------------------------------------
		//	ジョイパッドコントロールコード送信要求
		//	┏━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳━┳
		//	┃ 0┃ 1┃ 2┃ 3┃ 4┃ 5┃ 6┃ 7┃ 8┃ 9┃10┃11┃12┃13┃14┃15┃16┃　┃
		//	┗━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻━┻
		//	          ┗ 
		//--------------------------------------------------------------------------
		case MY::DEVICE_CONTROL::SEND_JOYPAD:
			break;

		default:
#ifdef DEBUG_MODE
			M5.Log.printf("------------------------------------------------------------------------------------------------------------\n");
#endif	//DEBUG_MODE
			break;
	}

	return(newMode);
}

//******************************************************************************
//	メディアコントロール処理
//	IN	mode:	メディアコントロールモード
//		msg:	ログ文字列へのポインタ
////	OUT:		true = 正常終了, false: エラー
//	OUT:		変更後のメディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procMediaControl(MY::MEDIA_CONTROL mode, const char* msg)
{
	bool bRet;

	uint8_t sendData[250 + 1];  // 送信可能250バイト +1
	esp_err_t result;
	uint8_t sendLen = 0;

	// 通常モードなら何もしない
	if((mode == MY::MEDIA_CONTROL::NONE) || (mode == MY::MEDIA_CONTROL::NORMAL))
	{
		// 本体・Atom・エンコーダーユニット・エンコーダーハットのLED制御、ブザーOFF
//		M5.Power.setLed(0);
		(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_ALL, my.LEDCOLOR_NORMAL);
		(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_NORMAL);
		M5.Speaker.stop();

		return(MY::MEDIA_CONTROL::NONE);
	}

	// LEDオン
//	M5.Power.setLed(64);
	// モード別処理
	switch(mode)
	{
		//--------------------------------------------------------------------------
		// (0x11)前のトラック
		//--------------------------------------------------------------------------
		case MY::MEDIA_CONTROL::PREVIOUS:
#ifdef DEBUG_MODE
			M5.Log.printf("- %s: Previous Track\n", msg);
#endif	//DEBUG_MODE
			// ブザーON、キーコード送信、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.tone(my.TONE_FREQ_PREVIOUS, my.TONE_DURATION);
#if defined(USE_USB_OTG)
			// USB制御
			usbHidKeyboardPress(CONSUMER_CONTROL_SCAN_PREVIOUS);
#endif	//defined(USE_USB_OTG)
			// 自身が Controllerの場合、Slaveにコマンド送信
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
				// 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_KEYCODE;	// キーコード送信
				sendData[sendLen++] = my.espMode;						// Slave/Controllerモード
				sendData[sendLen++] = mode;								// コントロールモード
#ifdef DEBUG_MODE
				// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
				M5.Log.printf("\t(S) キー「Previous Track」送信 data = [%02X,%02X,%02X,%02X,%02X,%02X] to (Idx=%d)\n",
					sendData[0], sendData[1], sendData[2], sendData[3], sendData[4], sendData[5],
					my.slaveIdx
				);
#endif	//DEBUG_MODE
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(my.slaveIdx, sendData, (sendLen + 1));
			}

			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_LEFT, my.LEDCOLOR_PREVIOUS);
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_RIGHT, my.LEDCOLOR_NORMAL);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_PREVIOUS);

			break;

		//--------------------------------------------------------------------------
		// (0x12)次のトラック
		//--------------------------------------------------------------------------
		case MY::MEDIA_CONTROL::NEXT:
#ifdef DEBUG_MODE
			M5.Log.printf("- %s: Next Track\n", msg);
#endif	//DEBUG_MODE
			// ブザーON、キーコード送信、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.tone(my.TONE_FREQ_NEXT, my.TONE_DURATION);
#if defined(USE_USB_OTG)
			// USB制御
			usbHidKeyboardPress(CONSUMER_CONTROL_SCAN_NEXT);
#endif	//defined(USE_USB_OTG)
			// 自身が Controllerの場合、Slaveにコマンド送信
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
				// 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_KEYCODE;	// キーコード送信
				sendData[sendLen++] = my.espMode;						// Slave/Controllerモード
				sendData[sendLen++] = mode;								// コントロールモード
#ifdef DEBUG_MODE
				// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
				M5.Log.printf("\t(S) キー「Next Track」送信 data = [%02X,%02X,%02X,%02X,%02X,%02X] to (Idx=%d)\n",
					sendData[0], sendData[1], sendData[2], sendData[3], sendData[4], sendData[5],
					my.slaveIdx
				);
#endif	//DEBUG_MODE
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(my.slaveIdx, sendData, (sendLen + 1));
			}

			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_LEFT, my.LEDCOLOR_NORMAL);
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_RIGHT, my.LEDCOLOR_NEXT);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_NEXT);

			break;

		//--------------------------------------------------------------------------
		// (0x13)再生/一時停止
		//--------------------------------------------------------------------------
		case MY::MEDIA_CONTROL::PAUSE:
#ifdef DEBUG_MODE
			M5.Log.printf("- %s: Pause\n", msg);
#endif	//DEBUG_MODE
			// ブザーON、キーコード送信、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.tone(my.TONE_FREQ_PAUSE, my.TONE_DURATION);
#if defined(USE_USB_OTG)
			// USB制御
			usbHidKeyboardPress(CONSUMER_CONTROL_PLAY_PAUSE);
#endif	//defined(USE_USB_OTG)
			// 自身が Controllerの場合、Slaveにコマンド送信
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
				// 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_KEYCODE;	// キーコード送信
				sendData[sendLen++] = my.espMode;						// Slave/Controllerモード
				sendData[sendLen++] = mode;								// コントロールモード
#ifdef DEBUG_MODE
				// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
				M5.Log.printf("\t(S) キー「Pause」送信 data = [%02X,%02X,%02X,%02X,%02X,%02X] to (Idx=%d)\n",
					sendData[0], sendData[1], sendData[2], sendData[3], sendData[4], sendData[5],
					my.slaveIdx
				);
#endif	//DEBUG_MODE
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(my.slaveIdx, sendData, (sendLen + 1));
			}
			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_ALL, my.LEDCOLOR_PAUSE);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_PAUSE);

			break;

		//--------------------------------------------------------------------------
		// (0x21)ボリュームダウン
		//--------------------------------------------------------------------------
		case MY::MEDIA_CONTROL::VDOWN:
#ifdef DEBUG_MODE
			M5.Log.printf("- %s: Volume DOWN\n", msg);
#endif	//DEBUG_MODE
			// ブザーON、キーコード送信、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.tone(my.TONE_FREQ_VDOWN, my.TONE_DURATION);
#if defined(USE_USB_OTG)
			// USB制御
			usbHidKeyboardPress(CONSUMER_CONTROL_VOLUME_DECREMENT);
#endif	//defined(USE_USB_OTG)
			// 自身が Controllerの場合、Slaveにコマンド送信
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
				// 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_KEYCODE;	// キーコード送信
				sendData[sendLen++] = my.espMode;						// Slave/Controllerモード
				sendData[sendLen++] = mode;								// コントロールモード
#ifdef DEBUG_MODE
				// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
				M5.Log.printf("\t(S) キー「Volume DOWN」送信 data = [%02X,%02X,%02X,%02X,%02X,%02X] to (Idx=%d)\n",
					sendData[0], sendData[1], sendData[2], sendData[3], sendData[4], sendData[5],
					my.slaveIdx
				);
#endif	//DEBUG_MODE
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(my.slaveIdx, sendData, (sendLen + 1));
			}
			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_LEFT, my.LEDCOLOR_VDOWN);
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_RIGHT, my.LEDCOLOR_NORMAL);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_VDOWN);
#if false
#if defined(__EspEasyLED_H__)
			led.setBrightness(20);
			led.showColor(255, 0, 0);
#endif	//defined(__EspEasyLED_H__)
#endif
			break;

		//--------------------------------------------------------------------------
		// (0x22)ボリュームアップ
		//--------------------------------------------------------------------------
		case MY::MEDIA_CONTROL::VUP:
#ifdef DEBUG_MODE
			M5.Log.printf("- %s: Volume UP\n", msg);
#endif	//DEBUG_MODE
			// ブザーON、キーコード送信、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.tone(my.TONE_FREQ_VUP, my.TONE_DURATION);
#if defined(USE_USB_OTG)
			// USB制御
			usbHidKeyboardPress(CONSUMER_CONTROL_VOLUME_INCREMENT);
#endif	//defined(USE_USB_OTG)
			// 自身が Controllerの場合、Slaveにコマンド送信
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
				// 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_KEYCODE;	// キーコード送信
				sendData[sendLen++] = my.espMode;						// Slave/Controllerモード
				sendData[sendLen++] = mode;								// コントロールモード
#ifdef DEBUG_MODE
				// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
				M5.Log.printf("\t(S) キー「Volume UP」送信 data = [%02X,%02X,%02X,%02X,%02X,%02X] to (Idx=%d)\n",
					sendData[0], sendData[1], sendData[2], sendData[3], sendData[4], sendData[5],
					my.slaveIdx
				);
#endif	//DEBUG_MODE
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(my.slaveIdx, sendData, (sendLen + 1));
			}
			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_LEFT, my.LEDCOLOR_NORMAL);
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_RIGHT, my.LEDCOLOR_VUP);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_VUP);
#if false
#if defined(__EspEasyLED_H__)
			led.setBrightness(20);
			led.showColor(0, 0, 255);
#endif	//defined(__EspEasyLED_H__)
#endif

			break;

		//--------------------------------------------------------------------------
		// (0x23)ミュート
		//--------------------------------------------------------------------------
		case MY::MEDIA_CONTROL::MUTE:
#ifdef DEBUG_MODE
			M5.Log.printf("- %s: Mute\n", msg);
#endif	//DEBUG_MODE
			// ブザーON、キーコード送信、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.tone(my.TONE_FREQ_MUTE1, my.TONE_DURATION);
#if defined(USE_USB_OTG)
			// USB制御
			usbHidKeyboardPress(CONSUMER_CONTROL_MUTE);
#endif	//defined(USE_USB_OTG)
			// 自身が Controllerの場合、Slaveにコマンド送信
			if(my.espMode == MY::ESP_MODE::CONTROLLER)
			{
				// 送信データ作成
				memset(sendData, 0x00, sizeof(sendData));
				sendLen = 0;
				sendData[sendLen++] = MY::DEVICE_CONTROL::SEND_KEYCODE;	// キーコード送信
				sendData[sendLen++] = my.espMode;						// Slave/Controllerモード
				sendData[sendLen++] = mode;								// コントロールモード
#ifdef DEBUG_MODE
				// ログは「関数名 : コマンドコード[データ] from 送信元MACアドレス」のフォーマットで出力すること
				M5.Log.printf("\t(S) キー「Mute」送信 data = [%02X,%02X,%02X,%02X,%02X,%02X] to (Idx=%d)\n",
					sendData[0], sendData[1], sendData[2], sendData[3], sendData[4], sendData[5],
					my.slaveIdx
				);
#endif	//DEBUG_MODE
				//*** ESP-NOW送信処理
				bRet = MyESPNOW::send(my.slaveIdx, sendData, (sendLen + 1));
			}
			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_ALL, my.LEDCOLOR_MUTE);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_MUTE);
#if false
#if defined(__EspEasyLED_H__)
			led.setBrightness(20);
			led.setColor(0, 0, 255, 0);
			led.setColor(1, 0, 0, 255);
			led.setColor(2, 255, 0, 0);
			led.setColor(3, 0, 255, 0);
			led.setColor(4, 0, 0, 255);
			led.setColor(5, 255, 0, 0);
			led.setColor(6, 0, 255, 0);
			led.setColor(7, 0, 0, 255);
			led.setColor(8, 255, 0, 0);
			led.setColor(9, 0, 255, 0);
			led.show();
#endif	//defined(__EspEasyLED_H__)
#endif
			// ミュート時には「ピコ」っと鳴らしたいので、少々 Waitしてから次の音を出す
			delay(my.DELAY_MUTE_DURATION);
			M5.Speaker.tone(my.TONE_FREQ_MUTE2, my.TONE_DURATION);

			break;

		//--------------------------------------------------------------------------
		// (0x41)モード変更
		//--------------------------------------------------------------------------
		case MY::MEDIA_CONTROL::MODE_CHANGE:
#ifdef DEBUG_MODE
			M5.Log.printf("- %s: Mode Change\n", msg);
#endif	//DEBUG_MODE
			// ブザーON、キーコード送信、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.tone(my.TONE_FREQ_MODE_CHANGE, my.TONE_DURATION);
			//***** USB制御/Slaveへのコマンド送信は行わず、内部のモード変更のみ
			// 音量 → トラック → スピード → 音量 → ・・・
			my.controlMode = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? MY::CONTROL_MODE::TRACK : ((my.controlMode == MY::CONTROL_MODE::TRACK) ? MY::CONTROL_MODE::SPEED :  MY::CONTROL_MODE::VOLUME));
			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_ALL, my.LEDCOLOR_MUTE);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_MUTE);

			break;

		//--------------------------------------------------------------------------
		// デフォルト
		//--------------------------------------------------------------------------
		default:
			// ブザーOFF、Atom・エンコーダーユニット・エンコーダーハットのLED制御
			M5.Speaker.stop();
			// RGB LED制御
			(my.oEncoderUnit)->setRGBLED(MY_I2C_ENCODER_UNIT::LED_ALL, my.LEDCOLOR_NORMAL);
			(my.oEncoderHat)->setRGBLED(my.LEDCOLOR_NORMAL);

			break;
	}
	// 本体のLEDオフ
//	M5.Power.setLed(0);
//	// ここまで来たら正常終了
//	bRet = true;

	// ここまで来たら正常終了として指定されたコントロールモードを返す
	return(mode);
}
