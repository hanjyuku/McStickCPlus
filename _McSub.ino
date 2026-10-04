//******************************************************************************
//	メディアコントロール サブ
//		2025/07/14:	メディアコントロールサーバーからメディアコントロールに改名。略称Mc
//		2025/08/02:	AtomLite/StickCを対象外に
//		2025/08/06: StickC系専用にデュアルボタンユニット用メソッドを追加
//		2025/08/31:	USB制御をサブルーチン化
//		2025/09/19:	各種サブルーチンの最適化
//		2025/09/23:	_McSub.inoからデバイス関連のみを抜き出して _McSubDevice.inoへ
//------------------------------------------------------------------------------
//	Notice:
//		Arduino IDEのソース分割は複数のinoファイルを単純にマージするだけ。
//		C/C++の分割コンパイルと違うからお手軽
//******************************************************************************
//*** for Tab5 WiFI.begin()
#define SDIO2_CLK GPIO_NUM_12
#define SDIO2_CMD GPIO_NUM_13
#define SDIO2_D0  GPIO_NUM_11
#define SDIO2_D1  GPIO_NUM_10
#define SDIO2_D2  GPIO_NUM_9
#define SDIO2_D3  GPIO_NUM_8
#define SDIO2_RST GPIO_NUM_15

//******************************************************************************
//	メディアコントロールモード名称
//******************************************************************************
const char* getMediaControlModeName(MY::MEDIA_CONTROL ctrlMode)
{
	return(((ctrlMode == MY::MEDIA_CONTROL::PREVIOUS) ? "前のトラック" :
		((ctrlMode == MY::MEDIA_CONTROL::NEXT) ? "次のトラック" :
		((ctrlMode == MY::MEDIA_CONTROL::PAUSE) ? "再生/一時停止" :
		((ctrlMode == MY::MEDIA_CONTROL::VUP) ? "音量アップ" :
		((ctrlMode == MY::MEDIA_CONTROL::VDOWN) ? "音量ダウン" :
		((ctrlMode == MY::MEDIA_CONTROL::MUTE) ? "ミュート" : NULL)))))));
}

//******************************************************************************
//	コマンドコード名称
//******************************************************************************
const char* getModeName(MY::DEVICE_CONTROL ctrlMode)
{
	return(((ctrlMode == MY::DEVICE_CONTROL::REQUEST_MAC) ? "MACアドレス要求" :
		((ctrlMode == MY::DEVICE_CONTROL::SEND_MAC) ? "MACアドレス通知" :
		((ctrlMode == MY::DEVICE_CONTROL::REQUEST_NAME) ? "ニックネーム要求" :
		((ctrlMode == MY::DEVICE_CONTROL::SEND_NAME) ? "ニックネーム通知" :
		((ctrlMode == MY::DEVICE_CONTROL::SEND_KEYCODE) ? "キーコード送信要求" :
		((ctrlMode == MY::DEVICE_CONTROL::SEND_MOUSE) ? "MOUSEコントロールコード送信要求" :
		((ctrlMode == MY::DEVICE_CONTROL::SEND_JOYPAD) ? "JOYPADコントロールコード送信要求" :
		((ctrlMode == MY::DEVICE_CONTROL::SEND_MOUSE) ? "MOUSEコントロールコード送信要求" : NULL)))))))));
}

//******************************************************************************
//	USB HID 準拠コンシュマー制御デバイスに指定されたキーコードを送る
//******************************************************************************
bool usbHidKeyboardPress(uint16_t _keycode)
{
	return(usbHidKeyboardPress(_keycode, true));
}

bool usbHidKeyboardPress(uint16_t _keycode, bool _release)
{
	size_t retP = 0;
#if defined(USE_USB_OTG)
	retP = consumerControl.press(_keycode);
	if(_release)
	{
		size_t retR = consumerControl.release();
	}
#endif	//defined(USE_USB_OTG)

	return(retP);
}

//------------------------------------------------------------------------------
//	Slaveチェック処理
//------------------------------------------------------------------------------
bool checkSlave(void)
{
#ifdef DEBUG_MODE
	M5.Log.printf("-------------------------------------------------------\n");
	M5.Log.printf("- checkSlave(): ESP-NOW Slaveチェック処理\n");
#endif	//DEBUG_MODE

	esp_err_t espErr;

	//*** マルチキャスト送信で Slaveに MACアドレスを要求
	//*** 結果は loop()の中で確認します
	uint8_t sendData[250 + 1];							// 送信可能250バイト +1
	int8_t sendLen = 0;									// 送信データ長
	// 送信データ作成
	memset(sendData, 0x00, sizeof(sendData));
	sendData[sendLen++] = MY::DEVICE_CONTROL::REQUEST_MAC;		// MACアドレスを要求
	sendData[sendLen++] = MY::ESP_MODE::NO_INIT;				// 現時点では Slave/Contorllerのどちらでもない
	memcpy(&sendData[sendLen], my.macAddr, sizeof(my.macAddr));	// 自身のMACアドレス通知
	sendLen += 6;
	memcpy(&sendData[sendLen], my.nickName, strlen(my.nickName));	// 自身のニックネーム通知
	sendLen += strlen(my.nickName);
#ifdef DEBUG_MODE
	M5.Log.printf("------------------------------------------------------------------------------------------------------------\n");
	M5.Log.printf("(S) MACアドレス要求: 電文内MACアドレス: [%02X:%02X:%02X:%02X:%02X:%02X]\n",
		*(sendData + 0), *(sendData + 1), *(sendData + 2), *(sendData + 3), *(sendData + 4), *(sendData + 5)
	);
#endif	//DEBUG_MODE
	// ESP-NOWデータ送信処理
	bool bRet = MyESPNOW::send(-1, sendData, (sendLen + 1));
#ifdef DEBUG_MODE
	M5.Log.printf("------------------------------------------------------------------------------------------------------------\n");
#endif	//DEBUG_MODE
	// 送信が成功したら、マルチキャスト送信で Slaveに MACアドレス要求を送信した時間をセット
	if(espErr == ESP_OK)
	{
		my.requestMacTime = millis();
		// 受信フラグ初期化
		MyESPNOW::setCalledRecv(false);
	}

	// ここまで来たら正常終了
	return((espErr == ESP_OK));
}

#if defined(DISP_MODE_CORE)	// Core系の場合
//******************************************************************************
//	時計表示 for Core系、Dial、Tab5
//		IN	bInit:	true = 初期化時, false = 時計表示時
//******************************************************************************
void drawClock(bool bInit)
{
	static long lOldMin = -99;
	static long lOldSec = -99;

	//*** 初期化時
	if(bInit)
	{
		//*** 全画面塗りつぶして、デバッグ用の格子を描画
#if defined(DISP_MODE_CORE)
		//*** Core系、Dial、Tab5の場合
		// 全面塗りつぶし(fillCircle()と fillArc()とで外枠を描画する座標が違うよ)
		M5.Display.fillArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS, 0, 0, 360, my.COLOR_BACK_MAIN);
#ifdef DEBUG_MODE
#if	defined(ARDUINO_M5STACK_TAB5)
		// 他機種との比較用
		// 横方向：320ドット区切りの格子
		for(int ii = 0; ii < 1280; ii += 320)
		{
			M5.Display.drawLine(ii, 0, ii, 1280, my.COLOR_GREEN);
		}
		// 縦方向：240ドット区切りの格子
		for(int ii = 0; ii < 1280; ii += 240)
		{
			M5.Display.drawLine(0, ii, 1280, ii, my.COLOR_GREEN);
		}
#endif	//defined(ARDUINO_M5STACK_TAB5)
		// 30ドット区切りの格子
		for(int ii = 0; ii < my.SCREEN_WIDTH; ii += 30)
		{
			M5.Display.drawLine(ii, 0, ii, my.SCREEN_WIDTH, my.COLOR_CYAN);
		}
		for(int ii = 0; ii < my.SCREEN_HEIGHT; ii += 30)
		{
			M5.Display.drawLine(0, ii, my.SCREEN_HEIGHT, ii, my.COLOR_CYAN);
		}
		// 表示エリア1：囲い
		M5.Display.drawRect(my.ar1.iXstt, my.ar1.iYstt, my.ar1.iWidth, my.ar1.iHeight, my.COLOR_DARKGREY);
		// 表示エリア1：罫線
		M5.Display.drawLine(0, my.ar1.iYstt, my.SCREEN_WIDTH, my.ar1.iYstt, my.COLOR_DARKGREY);
		M5.Display.drawLine(0, my.ar1.iXend, my.SCREEN_WIDTH, my.ar1.iXend, my.COLOR_DARKGREY);
		// 表示エリア2：囲い
		M5.Display.fillRect(my.ar2.iXstt, my.ar2.iYstt, my.ar2.iWidth, my.ar2.iHeight, my.COLOR_GREENYELLOW);
		M5.Display.drawRect(my.ar2.iXstt, my.ar2.iYstt, my.ar2.iWidth, my.ar2.iHeight, my.COLOR_GREEN);
		// 表示エリア2：罫線
		M5.Display.drawLine(0, my.ar2.iYstt, my.SCREEN_WIDTH, my.ar2.iYstt, my.COLOR_DARKGREY);
		M5.Display.drawLine(0, my.ar2.iYend, my.SCREEN_WIDTH, my.ar2.iYend, my.COLOR_DARKGREY);
		// 表示エリア3：囲い
		M5.Display.fillRect(my.ar3.iXstt, my.ar3.iYstt, my.ar3.iWidth, my.ar3.iHeight, my.COLOR_PINK);
		M5.Display.drawRect(my.ar3.iXstt, my.ar3.iYstt, my.ar3.iWidth, my.ar3.iHeight, my.COLOR_RED);
		// 表示エリア3：罫線
		M5.Display.drawLine(0, my.ar3.iYstt, my.SCREEN_WIDTH, my.ar3.iYstt, my.COLOR_DARKGREY);
		M5.Display.drawLine(0, my.ar3.iYend, my.SCREEN_WIDTH, my.ar3.iYend, my.COLOR_DARKGREY);
#endif	//DEBUG_MODE
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
		//*** StickC系、AtomS3の場合
		// 全面塗りつぶし
		M5.Display.fillRect(0, 0, 128, 128, my.COLOR_BACK_MAIN);
#ifdef DEBUG_MODE
		// 16ドット区切りの格子
		for(int ii = 0; ii < 128; ii += 16)
		{
			M5.Display.drawLine(ii, 0, ii, 127, my.COLOR_CYAN);
		}
		for(int ii = 0; ii < 128; ii += 16)
		{
			M5.Display.drawLine(0, ii, 127, ii, my.COLOR_CYAN);
		}
#endif	//DEBUG_MODE
#elif defined(DISP_MODE_COREINK)
		//*** CORE.INKの場合
#endif	//defined(DISP_MODE_CORE)

		// ESP-NOW Slave/Controllerモード表示
		drawEspMode(my.espMode);
	}

	//*** ESP32の内部タイマーから JST日時を取得(年の+1900補正が必要)
	auto timeBuff = time(nullptr);
	auto timeLocal = localtime(&timeBuff);

	//*** 前回描画より1秒以上経っていたら
	if(bInit || (timeLocal->tm_min != lOldMin) || (timeLocal->tm_sec != lOldSec))
	{
		//*** 時計の周り
		drawTimer(bInit, timeLocal->tm_sec);

		//*** 時計表示
		// 分が変わっていた時のみ
		if(bInit || (timeLocal->tm_min != lOldMin))
		{
#if true
			// バッテリー状態表示
			M5.Display.setFont(&fonts::AsciiFont8x16);
			M5.Display.setTextColor(my.COLOR_BLUE, my.COLOR_BLACK);
			M5.Display.setTextSize(1);
			M5.Display.setCursor(40 + 2, 180);
			M5.Display.printf("Level=%d,Vol=%d       ", M5.Power.getBatteryLevel(), M5.Power.getBatteryVoltage());
			M5.Display.setCursor(40 + 2, 200);
			M5.Display.printf("Brightness=%d   ", M5.Display.getBrightness());
#endif
			// 年月日
			M5.Display.setFont(my.FONT_CALENDER);
			M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
#if defined(DISP_MODE_CORE)
			M5.Display.setTextSize(my.FONTSIZE_CALENDER);
			M5.Display.setCursor(40 + 2, 60 + 2);
			M5.Display.printf("%04d/%02d/%02d(%s)", (timeLocal->tm_year + 1900), (timeLocal->tm_mon + 1), timeLocal->tm_mday, my.dow[timeLocal->tm_wday]);
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
			M5.Display.setTextSize(my.FONTSIZE_CALENDER_ATOMS3);
			M5.Display.setCursor(16, 32);
			M5.Display.printf("%02d/%02d(%s)", (timeLocal->tm_mon + 1), timeLocal->tm_mday, my.dow[timeLocal->tm_wday]);
#elif defined(DISP_MODE_COREINK)
#endif	//defined(DISP_MODE_CORE)
			// 時分の影を液晶モニタっぽく
			M5.Display.setFont(my.FONT_CLOCK);
			M5.Display.setTextColor(M5.Display.color888(184, 184, 184), TFT_LIGHTGREY);
#if defined(DISP_MODE_CORE)
			M5.Display.setTextSize(my.FONTSIZE_HHMM);
			M5.Display.setCursor(34, 90);
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
			M5.Display.setTextSize(my.FONTSIZE_HHMM_ATOMS3);
			M5.Display.setCursor(12, 48);
#elif defined(DISP_MODE_COREINK)
#endif	//defined(DISP_MODE_CORE)
			M5.Display.printf("88:88");
			// 時分
			M5.Display.setTextColor(TFT_BLACK);
#if defined(DISP_MODE_CORE)
			M5.Display.setCursor(34, 90);
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
			M5.Display.setCursor(12, 48);
#elif defined(DISP_MODE_COREINK)
#endif	//defined(DISP_MODE_CORE)
			M5.Display.printf("%02d:%02d", timeLocal->tm_hour, timeLocal->tm_min);
		}
		// 秒の影
		M5.Display.setFont(my.FONT_CLOCK);
		M5.Display.setTextColor(M5.Display.color888(184, 184, 184), TFT_LIGHTGREY);
#if defined(DISP_MODE_CORE)
		M5.Display.setTextSize(my.FONTSIZE_SS);
		M5.Display.setCursor(162, 152);
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
		M5.Display.setTextSize(my.FONTSIZE_SS_ATOMS3);
		M5.Display.setCursor(96, 84);
#elif defined(DISP_MODE_COREINK)
#endif	//defined(DISP_MODE_CORE)
		M5.Display.printf(":00");
		// 秒
		M5.Display.setTextColor(TFT_BLACK);
#if defined(DISP_MODE_CORE)
		M5.Display.setCursor(162, 152);
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
		M5.Display.setCursor(96, 84);
#elif defined(DISP_MODE_COREINK)
#endif	//defined(DISP_MODE_CORE)
		M5.Display.printf(":%02d", timeLocal->tm_sec);

		//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
		//*** エリア2：各種ステータス表示
		//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
		drawStatus(MY::STATUS_USBOTG);				// USB-OTGモード
		drawStatus(MY::STATUS_ESPMODE);				// ESP-NOW Slave/Controlleモード
		drawStatus(MY::STATUS_WIFI);				// WiFi接続状況
		drawStatus(MY::STATUS_CTRLMODE);			// コントロールモード
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
		drawStatus(MY::STATUS_HMI_MODULE);			// HMIモジュール
		drawStatus(MY::STATUS_ENCODER_MODULE);		// エンコーダーモジュール
#elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		drawStatus(MY::STATUS_MINI_ENCODER);		// MiniエンコーダーHAT
		drawStatus(MY::STATUS_MINI_JOYC);			// MiniジョイスティックHAT
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		drawStatus(MY::STATUS_ENCODER);				// エンコーダーUNIT
//		drawStatus(MY::STATUS_JOYSTICK);			// ジョイスティックUNIT
	}
	// 前回値保存
	lOldMin = timeLocal->tm_min;
	lOldSec = timeLocal->tm_sec;

	return;
}
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)	// StickC系/AtomS3の場合
//******************************************************************************
//	時計表示 for StickC系、AtomS3
//		IN	bInit:	true = 初期化時, false = 時計表示時
//******************************************************************************
void drawClock(bool bInit)
{
	static long lOldMin = -99;
	static long lOldSec = -99;

	//*** 初期化時
	if(bInit)
	{
		//*** 全画面塗りつぶして、デバッグ用の格子を描画
		// 全面塗りつぶし
		M5.Display.fillRect(0, 0, 128, 128, my.COLOR_BACK_MAIN);
#ifdef DEBUG_MODE
		// 16ドット区切りの格子
		for(int ii = 0; ii < 128; ii += 16)
		{
			M5.Display.drawLine(ii, 0, ii, 127, my.COLOR_CYAN);
		}
		for(int ii = 0; ii < 128; ii += 16)
		{
			M5.Display.drawLine(0, ii, 127, ii, my.COLOR_CYAN);
		}
#endif	//DEBUG_MODE

		// ESP-NOW Slave/Controllerモード表示
		drawEspMode(my.espMode);
	}

	//*** ESP32の内部タイマーから JST日時を取得(年の+1900補正が必要)
	auto timeBuff = time(nullptr);
	auto timeLocal = localtime(&timeBuff);

	//*** 前回描画より1秒以上経っていたら
	if(bInit || (timeLocal->tm_min != lOldMin) || (timeLocal->tm_sec != lOldSec))
	{
		//*** 時計の周り
		drawTimer(bInit, timeLocal->tm_sec);

		//*** 時計表示
		// 分が変わっていた時のみ
		if(bInit || (timeLocal->tm_min != lOldMin))
		{
			// 年月日
			M5.Display.setFont(my.FONT_CALENDER);
			M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
			M5.Display.setTextSize(my.FONTSIZE_CALENDER_ATOMS3);
			M5.Display.setCursor(16, 32);
			M5.Display.printf("%02d/%02d(%s)", (timeLocal->tm_mon + 1), timeLocal->tm_mday, my.dow[timeLocal->tm_wday]);
			// 時分の影を液晶モニタっぽく
			M5.Display.setFont(my.FONT_CLOCK);
			M5.Display.setTextColor(M5.Display.color888(184, 184, 184), TFT_LIGHTGREY);
			M5.Display.setTextSize(my.FONTSIZE_HHMM_ATOMS3);
			M5.Display.setCursor(12, 48);
			M5.Display.printf("88:88");
			// 時分
			M5.Display.setTextColor(TFT_BLACK);
			M5.Display.setCursor(12, 48);
			M5.Display.printf("%02d:%02d", timeLocal->tm_hour, timeLocal->tm_min);
		}
		// 秒の影
		M5.Display.setFont(my.FONT_CLOCK);
		M5.Display.setTextColor(M5.Display.color888(184, 184, 184), TFT_LIGHTGREY);
		M5.Display.setTextSize(my.FONTSIZE_SS_ATOMS3);
		M5.Display.setCursor(96, 84);
		M5.Display.printf(":00");
		// 秒
		M5.Display.setTextColor(TFT_BLACK);
		M5.Display.setCursor(96, 84);
		M5.Display.printf(":%02d", timeLocal->tm_sec);
#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		// AtomS3の場合は各種ステータス表示を行わない
		//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
		//*** エリア2：各種ステータス表示
		//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
		drawStatus(MY::STATUS_USBOTG);				// USB-OTGモード
		drawStatus(MY::STATUS_ESPMODE);				// ESP-NOW Slave/Controlleモード
		drawStatus(MY::STATUS_WIFI);				// WiFi接続状況
		drawStatus(MY::STATUS_CTRLMODE);			// コントロールモード
		drawStatus(MY::STATUS_MINI_ENCODER);		// MiniエンコーダーHAT
		drawStatus(MY::STATUS_MINI_JOYC);			// MiniジョイスティックHAT
		drawStatus(MY::STATUS_ENCODER);				// エンコーダーUNIT
//		drawStatus(MY::STATUS_JOYSTICK);			// ジョイスティックUNIT
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	}
	// 前回値保存
	lOldMin = timeLocal->tm_min;
	lOldSec = timeLocal->tm_sec;

	return;
}
#endif	//defined(DISP_MODE_CORE)

#if defined(DISP_MODE_CORE)	// Core系の場合
//******************************************************************************
//	エリア２：ステータス表示 for Core系、Dial、Tab5
//		IN	iPart:	ステータス列挙体(0から始まる)
//******************************************************************************
void drawStatus(MY::DRAW_STATUS iPart)
{
	int iWidth;
	int iHeight;
	int iXstt;
	int iYstt;
	int iRound;
	int textColor;
	int backColor;
	int lineColor;
	const char* msg;

#if defined(DISP_MODE_CORE)
	iWidth = (my.ar2.iWidth / 3);
	iHeight = (my.ar2.iHeight / 2);
	iXstt = (my.ar2.iXstt + (iWidth * (iPart / 2)));
	iYstt = (my.ar2.iYstt + (iHeight * (iPart % 2)));
	iRound = 3;
#elif defined(DISP_MODE_STICKC)
	iWidth = ((my.ar1.iWidth / 3) + 1);						// StickC系用調整値 +1
	iHeight = (my.ar1.iHeight / 3);
	iXstt = (my.ar1.iXstt + (iWidth * (iPart / 3)));
	iYstt = (my.ar1.iYstt + (iHeight * (iPart % 3)));
	iRound = 3;
#elif defined(DISP_MODE_ATOMS3)
	iWidth = (my.ar1.iWidth / 3);
	iHeight = (my.ar1.iHeight / 3);
	iXstt = (my.ar1.iXstt + (iWidth * (iPart / 3)));
	iYstt = (my.ar1.iYstt + (iHeight * (iPart % 3)));
	iRound = 3;
#elif defined(DISP_MODE_COREINK)
#endif	//defined(DISP_MODE_CORE)

	switch(iPart)
	{
		case MY::STATUS_USBOTG:
#if defined(USE_USB_OTG)
			textColor = my.COLOR_DARKGREY;
			backColor = my.COLOR_LIGHTGREY;
			lineColor = my.COLOR_DARKGREY;
#else
			textColor = my.COLOR_LIGHTGREY;
			backColor = my.COLOR_DARKGREY;
			lineColor = my.COLOR_BLACK;
#endif	//defined(USE_USB_OTG)
			msg = "OTG";
			break;
		case MY::STATUS_ESPMODE:
			textColor = ((my.espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
			backColor = ((my.espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
			lineColor = ((my.espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_BLACK : my.COLOR_DARKGREY);
			msg = ((my.espMode == MY::ESP_MODE::NO_INIT) ? "none" : ((my.espMode == MY::ESP_MODE::SLAVE) ? "SLV" : "CNT")); 
			break;
		case MY::STATUS_WIFI:
			textColor = (my.bWiFi ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
			backColor = (my.bWiFi ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
			lineColor = (my.bWiFi ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			msg = "WiFi";
			break;
		case MY::STATUS_CTRLMODE:
			textColor = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
			backColor = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
			lineColor = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			msg = ((my.controlMode == MY::VOLUME) ? "VOL" : ((my.controlMode == MY::TRACK) ? "TRAC" : "SPED"));
			break;
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
		//*** Core系独自のモジュール
		case MY::STATUS_HMI_MODULE:
			if(my.oHmiModule)
			{
				textColor = (my.oHmiModule->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
				backColor = (my.oHmiModule->isEnabled() ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
				lineColor = (my.oHmiModule->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			}
			else
			{
				textColor = my.COLOR_LIGHTGREY;
				backColor = my.COLOR_DARKGREY;
				lineColor = my.COLOR_BLACK;
			}
			msg = "HMI";
			break;
		case MY::STATUS_ENCODER_MODULE:
			if(my.oEncoderModule)
			{
				textColor = (my.oEncoderModule->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
				backColor = (my.oEncoderModule->isEnabled() ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
				lineColor = (my.oEncoderModule->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			}
			else
			{
				textColor = my.COLOR_LIGHTGREY;
				backColor = my.COLOR_DARKGREY;
				lineColor = my.COLOR_BLACK;
			}
			msg = "EncM";
			break;
#elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		//*** StickC系独自のハット
		case MY::STATUS_MINI_ENCODER:
			if(my.oEncoderHat)
			{
				textColor = (my.oEncoderHat->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
				backColor = (my.oEncoderHat->isEnabled() ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
				lineColor = (my.oEncoderHat->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			}
			else
			{
				textColor = my.COLOR_LIGHTGREY;
				backColor = my.COLOR_DARKGREY;
				lineColor = my.COLOR_BLACK;
			}
			msg = "EncH";
			break;
		case MY::STATUS_MINI_JOYC:
			if(my.oMiniJoycHat)
			{
				textColor = (my.oMiniJoycHat->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
				backColor = (my.oMiniJoycHat->isEnabled() ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
				lineColor = (my.oMiniJoycHat->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			}
			else
			{
				textColor = my.COLOR_LIGHTGREY;
				backColor = my.COLOR_DARKGREY;
				lineColor = my.COLOR_BLACK;
			}
			msg = "JoyH";
			break;
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
		//*** 各機種共通のユニット
		case MY::STATUS_ENCODER:
			if(my.oEncoderUnit)
			{
				textColor = (my.oEncoderUnit->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
				backColor = (my.oEncoderUnit->isEnabled() ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
				lineColor = (my.oEncoderUnit->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			}
			else
			{
				textColor = my.COLOR_LIGHTGREY;
				backColor = my.COLOR_DARKGREY;
				lineColor = my.COLOR_BLACK;
			}
			msg = "EncU";
			break;
	}

	// 角丸領域を塗り潰して
	M5.Display.fillRoundRect(iXstt, iYstt, iWidth, iHeight, iRound, backColor);
	// メッセージ文字列を描画して
	M5.Display.setFont(&fonts::Font0);
	M5.Display.setTextSize(1);
	M5.Display.setTextColor(textColor, backColor);
	M5.Display.setCursor((iXstt + 4), (iYstt + 4));
	M5.Display.printf("%s", msg);
	// 枠を描く
	M5.Display.drawRoundRect(iXstt, iYstt, iWidth, iHeight, iRound, lineColor);

	return;
}
#elif defined(DISP_MODE_STICKC)	// StickC系の場合。AtomS3ではステータス表示しないことにした
//******************************************************************************
//	エリア２：ステータス表示 for StickC系。Core系ではエリア1だが StickC系ではエリア4
//		IN	iPart:	ステータス列挙体(0から始まる)
//******************************************************************************
void drawStatus(MY::DRAW_STATUS iPart)
{
	int iWidth;
	int iHeight;
	int iXstt;
	int iYstt;
	int iRound;
	int textColor;
	int backColor;
	int lineColor;
	const char* msg;

	iWidth = ((my.ar4.iWidth / 3) + 1);						// StickC系用調整値 +1
	iHeight = (my.ar1.iHeight / 3);
	iXstt = (my.ar4.iXstt + (iWidth * (iPart / 3)));
	iYstt = (my.ar4.iYstt + (iHeight * (iPart % 3)));
	iRound = 3;

	switch(iPart)
	{
		case MY::STATUS_USBOTG:
			textColor = my.COLOR_LIGHTGREY;
			backColor = my.COLOR_DARKGREY;
			lineColor = my.COLOR_BLACK;
			msg = "OTG";
			break;
		case MY::STATUS_ESPMODE:
			textColor = ((my.espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
			backColor = ((my.espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
			lineColor = ((my.espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_BLACK : my.COLOR_DARKGREY);
			msg = ((my.espMode == MY::ESP_MODE::NO_INIT) ? "none" : ((my.espMode == MY::ESP_MODE::SLAVE) ? "SLV" : "CNT")); 
			break;
		case MY::STATUS_WIFI:
			textColor = (my.bWiFi ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
			backColor = (my.bWiFi ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
			lineColor = (my.bWiFi ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			msg = "WiFi";
			break;
		case MY::STATUS_CTRLMODE:
			textColor = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
			backColor = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
			lineColor = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			msg = ((my.controlMode == MY::VOLUME) ? "VOL" : ((my.controlMode == MY::TRACK) ? "TRAC" : "SPED"));
			break;
		//*** 各機種共通のユニット
		case MY::STATUS_ENCODER:
			if(my.oEncoderUnit)
			{
				textColor = (my.oEncoderUnit->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_LIGHTGREY);
				backColor = (my.oEncoderUnit->isEnabled() ? my.COLOR_LIGHTGREY : my.COLOR_DARKGREY);
				lineColor = (my.oEncoderUnit->isEnabled() ? my.COLOR_DARKGREY : my.COLOR_BLACK);
			}
			else
			{
				textColor = my.COLOR_LIGHTGREY;
				backColor = my.COLOR_DARKGREY;
				lineColor = my.COLOR_BLACK;
			}
			msg = "EncU";
			break;
	}

	// 角丸領域を塗り潰して
	M5.Display.fillRoundRect(iXstt, iYstt, iWidth, iHeight, iRound, backColor);
	// メッセージ文字列を描画して
	M5.Display.setFont(&fonts::Font0);
	M5.Display.setTextSize(1);
	M5.Display.setTextColor(textColor, backColor);
	M5.Display.setCursor((iXstt + 4), (iYstt + 4));
	M5.Display.printf("%s", msg);
	// 枠を描く
	M5.Display.drawRoundRect(iXstt, iYstt, iWidth, iHeight, iRound, lineColor);

	return;
}
#endif	//defined(DISP_MODE_CORE)

//******************************************************************************
//	ESP-NOW Slave/Controllerモードに応じた枠描画
//		IN	espMode:	ESP-NOW Slave/Controllerモード
//******************************************************************************
void drawEspMode(MY::ESP_MODE espMode)
{
#ifdef DEBUG_MODE
//	M5.Log.printf("*** BLE接続状態変化：%d\n", bBleState);
#endif	//DEBUG_MODE

#if defined(DISP_MODE_CORE)
	// 円弧
	M5.Display.fillArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS, my.R_RADIUS2, 0, 360, ((espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_MODE_INIT : ((espMode == MY::ESP_MODE::SLAVE) ? my.COLOR_MODE_SLAVE : my.COLOR_MODE_CONTROLLER)));
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
	// 四角い枠
	M5.Display.drawRect(8, 8, 112, 112, ((espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_MODE_INIT : ((espMode == MY::ESP_MODE::SLAVE) ? my.COLOR_MODE_SLAVE : my.COLOR_MODE_CONTROLLER)));
	M5.Display.drawRect(9, 9, 110, 110, ((espMode == MY::ESP_MODE::NO_INIT) ? my.COLOR_MODE_INIT : ((espMode == MY::ESP_MODE::SLAVE) ? my.COLOR_MODE_SLAVE : my.COLOR_MODE_CONTROLLER)));
#elif defined(DISP_MODE_COREINK)
#endif	//defined(DISP_MODE_CORE)

	return;
}

//******************************************************************************
//	タイマーの秒の値に応じた枠描画
//		IN	bInit:		true = 初期化時, false = 時計表示時
//			iSecond:	時計の秒
//******************************************************************************
void drawTimer(bool bInit, uint32_t iSecond)
{
#if defined(DISP_MODE_CORE)
	//*** Core系、Dialの場合
	int32_t angle;

	// 初期化時には全周描画(指定秒までoff、それ以降on)
	if(bInit)
	{
		for(uint32_t ii = 1; ii <= 60; ii++)
		{
			angle = (((ii - 1) * 6) + my.CORRECTION_RADIUS);
			angle += ((angle < 0) ? 360 : 0);
			M5.Display.fillArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS2, my.R_RADIUS3, angle, (angle + 6), ((ii < iSecond) ? my.COLOR_DRAW_SECOND : my.COLOR_DRAW_INIT));
			M5.Display.drawArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS2, my.R_RADIUS3, angle, (angle + 6), ((ii < iSecond) ? my.COLOR_LINE_SECOND : my.COLOR_LINE_INIT));
		}
	}
	// 0秒ちょうどなら全周描画(すべてoff)
	else if(iSecond == 0)
	{
		for(uint32_t ii = 0; ii < 60; ii++)
		{
			angle = ((ii * 6) + my.CORRECTION_RADIUS);
			angle += ((angle < 0) ? 360 : 0);
			M5.Display.fillArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS2, my.R_RADIUS3, angle, (angle + 6), my.COLOR_DRAW_INIT);
			M5.Display.drawArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS2, my.R_RADIUS3, angle, (angle + 6), my.COLOR_LINE_INIT);
		}
	}
	// 0秒以外ならそこだけ描画(on)
	else
	{
		angle = (((iSecond - 1) * 6) + my.CORRECTION_RADIUS);
		angle += ((angle < 0) ? 360 : 0);
		M5.Display.fillArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS2, my.R_RADIUS3, angle, (angle + 6), my.COLOR_DRAW_SECOND);
		M5.Display.drawArc(my.X_CENTER, my.Y_CENTER, my.R_RADIUS2, my.R_RADIUS3, angle, (angle + 6), my.COLOR_LINE_SECOND);
	}
#if true	// 内周と外周に線
	M5.Display.drawCircle(my.X_CENTER, my.Y_CENTER, my.R_RADIUS2, my.COLOR_LINE_INIT);
	M5.Display.drawCircle(my.X_CENTER, my.Y_CENTER, my.R_RADIUS3, my.COLOR_LINE_INIT);
#endif
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
	//*** StickC系、AtomS3の場合
	int32_t xx, yy, ww, hh;
	int32_t sttX, sttY, endX, endY;

	// 初期化時には全周描画(指定秒までoff、それ以降on)
	if(bInit)
	{
		M5.Display.fillRect(8, 0, 120, 8, my.COLOR_DRAW_INIT);
		M5.Display.fillRect(120, 8, 8, 120, my.COLOR_DRAW_INIT);
		M5.Display.fillRect(0, 120, 120, 8, my.COLOR_DRAW_INIT);
		M5.Display.fillRect(0, 0, 8, 120, my.COLOR_DRAW_INIT);
		//
		for(uint32_t ii = 1; ii <= 60; ii++)
		{
			sttX = ((ii <= 15) ? (ii * 8) : ((ii <= 30) ? 120 : ((ii <= 45) ? (128 - (ii - 30) * 8) : 0)));
			sttY = ((ii <= 15) ? 0 : ((ii <= 30) ? ((ii - 15) * 8) : (ii <= 45) ? 120 : (128 - (ii - 45) * 8)));
			endX = ((ii <= 15) ? sttX : ((ii <= 30) ? 127 : ((ii <= 45) ? sttX : 7)));
			endY = ((ii <= 15) ? 7 : ((ii <= 30) ? sttY: ((ii <= 45) ? 127 : sttY)));
			M5.Display.drawLine(sttX, sttY, endX, endY, my.COLOR_LINE_INIT);
		}
	}
	// 0秒ちょうどなら全周描画(すべてoff)
	else if(iSecond == 0)
	{
		M5.Display.fillRect(8, 0, 120, 8, my.COLOR_DRAW_INIT);
		M5.Display.fillRect(120, 8, 8, 120, my.COLOR_DRAW_INIT);
		M5.Display.fillRect(0, 120, 120, 8, my.COLOR_DRAW_INIT);
		M5.Display.fillRect(0, 0, 8, 120, my.COLOR_DRAW_INIT);
		//
		for(uint32_t ii = 1; ii <= 60; ii++)
		{
			sttX = ((ii <= 15) ? (ii * 8) : ((ii <= 30) ? 120 : ((ii <= 45) ? (128 - (ii - 30) * 8) : 0)));
			sttY = ((ii <= 15) ? 0 : ((ii <= 30) ? ((ii - 15) * 8) : (ii <= 45) ? 120 : (128 - (ii - 45) * 8)));
			endX = ((ii <= 15) ? sttX : ((ii <= 30) ? 127 : ((ii <= 45) ? sttX : 8)));
			endY = ((ii <= 15) ? 7 : ((ii <= 30) ? sttY: ((ii <= 45) ? 127 : sttY)));
			M5.Display.drawLine(sttX, sttY, endX, endY, my.COLOR_LINE_INIT);
		}
	}
	// 0秒以外ならそこだけ描画(on)
	else
	{
		uint16_t ii = iSecond;
//		M5.Display.fillRect(8, 0, 120, 8, my.COLOR_DARKGREY);
//		M5.Display.fillRect(120, 8, 8, 120, my.COLOR_DARKGREY);
//		M5.Display.fillRect(0, 120, 120, 8, my.COLOR_DARKGREY);
//		M5.Display.fillRect(0, 0, 8, 120, my.COLOR_DARKGREY);
		//
		sttX = ((ii <= 15) ? (ii * 8) : ((ii <= 30) ? 120 : ((ii <= 45) ? (120 - (ii - 30) * 8) : 0)));
		sttY = ((ii <= 15) ? 0 : ((ii <= 30) ? ((ii - 15) * 8) : (ii <= 45) ? 120 : (120 - (ii - 45) * 8)));
		M5.Display.fillRect(sttX, sttY, 8, 8, my.COLOR_DRAW_SECOND);
		sttX = ((ii <= 15) ? (ii * 8) : ((ii <= 30) ? 120 : ((ii <= 45) ? (128 - (ii - 30) * 8) : 0)));
		sttY = ((ii <= 15) ? 0 : ((ii <= 30) ? ((ii - 15) * 8) : ((ii <= 45) ? 120 : (128 - (ii - 45) * 8))));
		endX = ((ii <= 15) ? (ii * 8) : ((ii <= 30) ? 127 : ((ii <= 45) ? (128 - (ii - 30) * 8) : 7)));
		endY = ((ii <= 15) ? 7 : ((ii <= 30) ? ((ii - 15) * 8) : ((ii <= 45) ? 127 : (128 - (ii - 45) * 8))));
		M5.Display.drawLine(sttX, sttY, endX, endY, my.COLOR_LINE_SECOND);
	}
#if false	// 内周と外周に線
	M5.Display.drawRect(0, 0, 128, 128, my.BACKCOLOR_MAIN);
	M5.Display.drawRect(7, 7, 114, 114, my.COLOR_DARKGREY);
#endif
#elif defined(DISP_MODE_COREINK)
	//*** CORE.INKの場合
#endif	//defined(DISP_MODE_CORE)

	return;
}

//******************************************************************************
//	NTPより現在日時を取得
//		IN:		void
//		OUT:	true = 正常終了, false: エラー
//******************************************************************************
bool getNtpDateTime(void)
{
	bool bRet = false;
	bool bWiFiEnabled = false;
	bool bNtpEnabled = false;

#ifdef DEBUG_MODE
	M5.Log.printf("-------------------------------------------------------\n");
	M5.Log.printf("- getNtpDateTime() : start ---\n");
#endif	//DEBUG_MODE
	//*** RTCチェック
#ifdef DEBUG_MODE
	M5.Log.printf("- RTC:");
	M5.Display.printf("- RTC:");
#endif	//DEBUG_MODE
	if(M5.Rtc.isEnabled())
	{
#ifdef DEBUG_MODE
		M5.Log.printf(" found.\n");
		M5.Display.printf(" found.\n");
#endif	//DEBUG_MODE
	}
	else
	{
#ifdef DEBUG_MODE
		M5.Log.printf(" not found.\n");
		M5.Display.printf(" not found.\n");
#endif	//DEBUG_MODE
	}

#ifdef DEBUG_MODE
	//*** RTCから UTC日時取得(年の+1900補正がいらない)
	auto dt = M5.Rtc.getDateTime();
	M5.Log.printf("- 1st get RTC(UTC) : %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
		dt.date.year, dt.date.month, dt.date.date, my.dow[dt.date.weekDay],
		dt.time.hours, dt.time.minutes, dt.time.seconds);
	M5.Display.printf("- 1st get RTC(UTC) : %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
		dt.date.year, dt.date.month, dt.date.date, my.dow[dt.date.weekDay],
		dt.time.hours, dt.time.minutes, dt.time.seconds);
#endif	//DEBUG_MODE

	//*** WiFi接続
#ifdef DEBUG_MODE
	M5.Log.printf("- WiFi:");
	M5.Display.printf("- WiFi:");
#endif	//DEBUG_MODE
#if	defined(ARDUINO_M5STACK_TAB5)
//	WiFi.setPins(SDIO2_CLK, SDIO2_CMD, SDIO2_D0, SDIO2_D1, SDIO2_D2, SDIO2_D3, SDIO2_RST);
//	 WiFi.mode(WIFI_STA);
#endif	//defined(ARDUINO_M5STACK_TAB5)
	WiFi.begin();
	for(uint32_t ii = 0; ii < my.RETRY_WIFI; ii++)
	{
		// WiFiに接続できた
		if(WiFi.status() == WL_CONNECTED)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("Connected.\n");
			M5.Display.printf("Connected.\n");
#endif	//DEBUG_MODE
			bWiFiEnabled = true;
			break;
		}
		// リトライ
#ifdef DEBUG_MODE
		M5.Log.printf(".");
		M5.Display.printf(".");
#endif	//DEBUG_MODE
		delay(my.DELAY_WIFI);
	}

	// WiFiに接続できたなら NTPから現在日時取得
	if(bWiFiEnabled)
	{
#ifdef DEBUG_MODE
		M5.Log.printf("- NTP:");
		M5.Display.printf("- NTP:");
#endif	//DEBUG_MODE
		//*** NTPから現在日時取得
		configTzTime(my.NTP_TIMEZONE, my.NTP_SERVER1, my.NTP_SERVER2, my.NTP_SERVER3);
		delay(1600);
		struct tm timeInfo;
		for(uint32_t ii = 0; ii < my.RETRY_WIFI; ii++)
		{
			if(getLocalTime(&timeInfo, 1000))
			{
#ifdef DEBUG_MODE
				M5.Log.printf(" Connected.\n");
				M5.Display.printf(" Connected.\n");
#endif	//DEBUG_MODE
				bNtpEnabled = true;
				break;
			}
#ifdef DEBUG_MODE
			M5.Log.printf(".");
			M5.Display.printf(".");
#endif	//DEBUG_MODE
		}
	}
	// WiFiに接続できなかった
	else
	{
#ifdef DEBUG_MODE
		M5.Log.printf(" Not Connected.\n");
		M5.Display.printf(" Not Connected.\n");
#endif	//DEBUG_MODE
		delay(my.DELAY_WIFI);
	}

	//*** 現在日時をRTCに設定
	// WiFiに接続できて NTPから現在日時取得できていたら
	if(bWiFiEnabled && bNtpEnabled)
	{
#ifdef DEBUG_MODE
		M5.Log.printf("- WiFi && NTP OK\n");
		M5.Display.printf("- WiFi && NTP OK\n");
#endif	//DEBUG_MODE
		// 現在日時から秒が切り替わった瞬間に、その日時を RTCに設定
		time_t t = time(nullptr) + 1;
		while (t > time(nullptr))
			;
		M5.Rtc.setDateTime(gmtime(&t));
		bRet = true;
	}
	// WiFiに接続できなかったか NTPから現在日時取得できなかったら
	else
	{
#ifdef DEBUG_MODE
		M5.Log.printf("- WiFi or NTP NG\n");
		M5.Display.printf("- WiFi or NTP NG\n");
#endif	//DEBUG_MODE
		M5.Rtc.setDateTime( { { 2024, 1, 1 }, { 0, 0, 0 } } );
	}
#ifdef DEBUG_MODE
	M5.Log.printf("- getNtpDateTime() : end\n");
	M5.Log.printf("-------------------------------------------------------\n");
#endif	//DEBUG_MODE

	return(bRet);
}

#if defined(DISP_MODE_CORE)	// Core系の場合
//******************************************************************************
//	表示エリアの座標計算 for Core/Dial
//		IN:		void
//		OUTl:	true = 正常終了, false: エラー
//******************************************************************************
bool calcCoordinate(void)
{
	bool bRet = false;
	double dTmp, dTmp2;

#ifdef DEBUG_MODE
//	M5.Log.printf("-------------------------------------------------------\n");
//	M5.Log.printf("- calcCoordinate() : start\n");
#endif	//DEBUG_MODE

#if defined(DISP_MODE_CORE)
	//*** Core系、Dial、Tab5の場合
	// 表示エリア1：x^2 + y^2 = r^2より、yとrの値から xを求める
	dTmp = pow(my.R_RADIUS3, 2);							// r^2
	dTmp2 = pow((my.H_AREA1 / 2), 2);						// y^2
	dTmp = sqrt(dTmp - dTmp2);								// x
	//
	my.ar1.iWidth = (dTmp * 2);								// 幅
	my.ar1.iHeight = my.H_AREA1;							// 高さ
	//
	my.ar1.iXstt = (my.X_CENTER - dTmp);					// 開始位置：x
	my.ar1.iYstt = (my.Y_CENTER - (my.H_AREA1 / 2));		// 開始位置：y
	my.ar1.iXend = (my.ar1.iXstt + my.ar1.iWidth);			// 終了位置：x
	my.ar1.iXend = (my.ar1.iYstt + my.ar1.iHeight);			// 終了位置：y

	// 表示エリア2：x^2 + y^2 = r^2より、yとrの値から xを求める
	dTmp = pow(my.R_RADIUS3, 2);							// r^2
	dTmp2 = pow(((my.H_AREA1 / 2) + my.H_AREA2), 2);		// y^2
	dTmp = sqrt(dTmp - dTmp2);								// x
	//
	my.ar2.iWidth = (dTmp * 2);								// 幅
	my.ar2.iHeight = my.H_AREA2;							// 高さ
	//
	my.ar2.iXstt = (my.X_CENTER - dTmp);							// 開始位置：x
	my.ar2.iYstt = (my.Y_CENTER - ((my.H_AREA1 / 2) + my.H_AREA2));	// 開始位置：y
	my.ar2.iXend = (my.ar2.iXstt + my.ar2.iWidth);					// 終了位置：x
	my.ar2.iYend = (my.ar2.iYstt + my.ar2.iHeight);					// 終了位置：y

	// 表示エリア3：x^2 + y^2 = r^2より、yとrの値から xを求める
	dTmp = pow(my.R_RADIUS3, 2);							// r^2
	dTmp2 = pow(((my.H_AREA1 / 2) + my.H_AREA3), 2);		// y^2
	dTmp = sqrt(dTmp - dTmp2);								// x
	//
	my.ar3.iWidth = (dTmp * 2);								// 幅
	my.ar3.iHeight = my.H_AREA3;							// 高さ
	//
	my.ar3.iXstt = (my.X_CENTER - dTmp);					// 開始位置：x
	my.ar3.iYstt = (my.Y_CENTER + (my.H_AREA1 / 2));		// 開始位置：y
	my.ar3.iXend = (my.ar3.iXstt + my.ar3.iWidth);			// 終了位置：x
	my.ar3.iYend = (my.ar3.iYstt + my.ar3.iHeight);			// 終了位置：y
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)
	//*** StickC系/AtomS3の場合
	// 表示エリア1
	my.ar1.iWidth = my.SCREEN_WIDTH_ATOMS3;					// 幅
	my.ar1.iHeight = (my.SCREEN_HEIGHT_ATOMS3 / 3);			// 高さ
	//
	my.ar1.iXstt = 0;										// 開始位置：x
	my.ar1.iYstt = 0;										// 開始位置：y
	my.ar1.iXend = (my.ar1.iXstt + my.ar1.iWidth);			// 終了位置：x
	my.ar1.iYend = (my.ar1.iYstt + my.ar1.iHeight);			// 終了位置：y

	// 表示エリア2
	my.ar2.iWidth = my.SCREEN_WIDTH_ATOMS3;					// 幅
	my.ar2.iHeight = (my.SCREEN_HEIGHT_ATOMS3 / 3);			// 高さ
	//
	my.ar2.iXstt = 0;										// 開始位置：x
	my.ar2.iYstt = (my.ar1.iYstt + my.ar1.iHeight);			// 開始位置：y
	my.ar2.iXend = (my.ar2.iXstt + my.ar2.iWidth);			// 終了位置：x
	my.ar2.iYend = (my.ar2.iYstt + my.ar2.iHeight);			// 終了位置：y

	// 表示エリア3
	my.ar3.iWidth = my.SCREEN_WIDTH_ATOMS3;					// 幅
	my.ar3.iHeight = (my.SCREEN_HEIGHT_ATOMS3 / 3);			// 高さ
	//
	my.ar3.iXstt = 0;										// 開始位置：x
	my.ar3.iYstt = (my.ar2.iYstt + my.ar2.iHeight);			// 開始位置：y
	my.ar3.iXend = (my.ar3.iXstt + my.ar3.iWidth);			// 終了位置：x
	my.ar3.iYend = (my.ar3.iYstt + my.ar3.iHeight);			// 終了位置：y
#elif defined(ARDUINO_M5STACK_COREINK)
	//*** CORE.INKの場合
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3) || defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_TAB5)

	return(bRet);
}
#elif defined(DISP_MODE_STICKC) || defined(DISP_MODE_ATOMS3)	// StickC系/AtomS3の場合
//******************************************************************************
//	表示エリアの座標計算 for StickC系/AtomS3
//		IN:		void
//		OUTl:	true = 正常終了, false: エラー
//******************************************************************************
bool calcCoordinate(void)
{
	bool bRet = false;
	double dTmp, dTmp2;

	// 表示エリア1
	my.ar1.iWidth = my.SCREEN_WIDTH_ATOMS3;					// 幅
	my.ar1.iHeight = (my.SCREEN_HEIGHT_ATOMS3 / 3);			// 高さ
	//
	my.ar1.iXstt = 0;										// 開始位置：x
	my.ar1.iYstt = 0;										// 開始位置：y
	my.ar1.iXend = (my.ar1.iXstt + my.ar1.iWidth);			// 終了位置：x
	my.ar1.iYend = (my.ar1.iYstt + my.ar1.iHeight);			// 終了位置：y

	// 表示エリア2
	my.ar2.iWidth = my.SCREEN_WIDTH_ATOMS3;					// 幅
	my.ar2.iHeight = (my.SCREEN_HEIGHT_ATOMS3 / 3);			// 高さ
	//
	my.ar2.iXstt = 0;										// 開始位置：x
	my.ar2.iYstt = (my.ar1.iYstt + my.ar1.iHeight);			// 開始位置：y
	my.ar2.iXend = (my.ar2.iXstt + my.ar2.iWidth);			// 終了位置：x
	my.ar2.iYend = (my.ar2.iYstt + my.ar2.iHeight);			// 終了位置：y

	// 表示エリア3
	my.ar3.iWidth = my.SCREEN_WIDTH_ATOMS3;					// 幅
	my.ar3.iHeight = (my.SCREEN_HEIGHT_ATOMS3 / 3);			// 高さ
	//
	my.ar3.iXstt = 0;										// 開始位置：x
	my.ar3.iYstt = (my.ar2.iYstt + my.ar2.iHeight);			// 開始位置：y
	my.ar3.iXend = (my.ar3.iXstt + my.ar3.iWidth);			// 終了位置：x
	my.ar3.iYend = (my.ar3.iYstt + my.ar3.iHeight);			// 終了位置：y

#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	// AtomS3の場合は表示エリア4はない
	// 表示エリア4
	my.ar4.iWidth = my.SCREEN_WIDTH_ATOMS3;					// 幅
	my.ar4.iHeight = my.SCREEN_HEIGHT_ATOMS3;				// 高さ
	//
	my.ar4.iXstt = 0;										// 開始位置：x
	my.ar4.iYstt = my.SCREEN_HEIGHT_ATOMS3;					// 開始位置：y
	my.ar4.iXend = (my.ar4.iXstt + my.ar4.iWidth);			// 終了位置：x
	my.ar4.iYend = (my.ar4.iYstt + my.ar4.iHeight);			// 終了位置：y
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)

	return(bRet);
}
#endif	//defined(DISP_MODE_CORE)

//******************************************************************************
//	board_t列挙体の値からボード名を取
//		IN:		board_t		ボード番号
//		OUTl:	const char*	ボード名へのポインタ
//******************************************************************************
const char* getBoardName(m5::board_t boardNo)
{
	const char* pName = "UNKNOWN";

	switch(boardNo)
	{
		// ディスプレイ付きボード
		case m5::board_t::board_M5Stack: pName = "BOARD_M5STACK"; break;
		case m5::board_t::board_M5StackCore2: pName = "BOARD_M5STACKCORE2"; break;
		case m5::board_t::board_M5StickC: pName = "BOARD_M5STICKC"; break;
		case m5::board_t::board_M5StickCPlus: pName = "BOARD_M5STICKCPLUS"; break;
		case m5::board_t::board_M5StickCPlus2: pName = "BOARD_M5STICKCPLUS2"; break;
		case m5::board_t::board_M5StackCoreInk: pName = "BOARD_M5STACKCOREINK"; break;
		case m5::board_t::board_M5Paper: pName = "BOARD_M5PAPER"; break;
		case m5::board_t::board_M5Tough: pName = "BOARD_M5TOUGH"; break;
		case m5::board_t::board_M5Station: pName = "BOARD_M5STATION"; break;
		case m5::board_t::board_M5StackCoreS3: pName = "BOARD_M5STACKCORES3"; break;
		case m5::board_t::board_M5AtomS3: pName = "BOARD_M5ATOMS3"; break;
		case m5::board_t::board_M5Dial: pName = "BOARD_M5DIAL"; break;
		case m5::board_t::board_M5DinMeter: pName = "BOARD_M5DINMETER"; break;
		case m5::board_t::board_M5Cardputer: pName = "BOARD_M5CARDPUTER"; break;
		case m5::board_t::board_M5AirQ: pName = "BOARD_M5AIRQ"; break;
		case m5::board_t::board_M5VAMeter: pName = "BOARD_M5VAMETER"; break;
		case m5::board_t::board_M5StackCoreS3SE: pName = "BOARD_M5STACKCORES3SE"; break;
		case m5::board_t::board_M5AtomS3R: pName = "BOARD_M5ATOMS3R"; break;
		case m5::board_t::board_M5PaperS3: pName = "BOARD_M5PAPERS3"; break;
		case m5::board_t::board_M5CoreMP135: pName = "BOARD_M5COREMP135"; break;
		case m5::board_t::board_M5StampPLC: pName = "BOARD_M5STAMPPLC"; break;
		case m5::board_t::board_M5Tab5: pName = "BOARD_M5TAB5"; break;
		// ディスプレイなしボード
		case m5::board_t::board_M5AtomLite: pName = "BOARD_M5ATOMLITE"; break;
		case m5::board_t::board_M5AtomPsram: pName = "BOARD_M5ATOMPSRAM"; break;
		case m5::board_t::board_M5AtomU: pName = "BOARD_M5ATOMU"; break;
		case m5::board_t::board_M5Camera: pName = "BOARD_M5CAMERA"; break;
		case m5::board_t::board_M5TimerCam: pName = "BOARD_M5TIMERCAM"; break;
		case m5::board_t::board_M5StampPico: pName = "BOARD_M5STAMPPICO"; break;
		case m5::board_t::board_M5StampC3: pName = "BOARD_M5STAMPC3"; break;
		case m5::board_t::board_M5StampC3U: pName = "BOARD_M5STAMPC3U"; break;
		case m5::board_t::board_M5StampS3: pName = "BOARD_M5STAMPS3"; break;
		case m5::board_t::board_M5AtomS3Lite: pName = "BOARD_M5ATOMS3LITE"; break;
		case m5::board_t::board_M5AtomS3U: pName = "BOARD_M5ATOMS3U"; break;
		case m5::board_t::board_M5Capsule: pName = "BOARD_M5CAPSULE"; break;
		case m5::board_t::board_M5NanoC6: pName = "BOARD_M5NANOC6"; break;
		case m5::board_t::board_M5AtomMatrix: pName = "BOARD_M5ATOMMATRIX"; break;
		case m5::board_t::board_M5AtomEcho: pName = "BOARD_M5ATOMECHO"; break;
		case m5::board_t::board_M5AtomS3RExt: pName = "BOARD_M5ATOMS3REXT"; break;
		case m5::board_t::board_M5AtomS3RCam: pName = "BOARD_M5ATOMS3RCAM"; break;
		// 外部ディスプレイ
		case m5::board_t::board_M5AtomDisplay: pName = "BOARD_M5ATOMDISPLAY"; break;
		case m5::board_t::board_M5UnitLCD: pName = "BOARD_M5UNITLCD"; break;
		case m5::board_t::board_M5UnitOLED: pName = "BOARD_M5UNITOLED"; break;
		case m5::board_t::board_M5UnitMiniOLED: pName = "BOARD_M5UNITMINIOLED"; break;
		case m5::board_t::board_M5UnitGLASS: pName = "BOARD_M5UNITGLASS"; break;
		case m5::board_t::board_M5UnitGLASS2: pName = "BOARD_M5UNITGLASS2"; break;
		case m5::board_t::board_M5UnitRCA: pName = "BOARD_M5UNITRCA"; break;
		case m5::board_t::board_M5ModuleDisplay: pName = "BOARD_M5MODULEDISPLAY"; break;
		case m5::board_t::board_M5ModuleRCA: pName = "BOARD_M5MODULERCA"; break;
		// 未定義
		default: pName= "UNKNOWN"; break;
	}

	return(pName);
}
