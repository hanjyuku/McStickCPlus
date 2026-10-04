//******************************************************************************
//	メディアコントロール サブ(デバイス関連)
//		2025/09/23:	_McSub.inoからデバイス関連のみを抜き出して _McSubDevice.inoへ
//------------------------------------------------------------------------------
//	Notice:
//		Arduino IDEのソース分割は複数のinoファイルを単純にマージするだけ。
//		C/C++の分割コンパイルと違うからお手軽
//******************************************************************************

//******************************************************************************
//	RBG LED制御
//******************************************************************************
int setRGBLED(int _color)
{
	int retColor = -1;

#if defined(ARDUINO_M5STACK_ATOMS3)							// for AtomS3/AtomS3U
	AtomS3.dis.drawpix(_color);
	AtomS3.dis.show();

	retColor = _color;
#endif	//defined(ARDUINO_M5STACK_ATOMS3)

	return(retColor);
}

#if false
//******************************************************************************
//	RGB LED設定 for ATOM Lite
//		IN		color:	カラーコード
//******************************************************************************
void setAtomRGBLED(uint32_t color)
{
#if defined(ARDUINO_M5STACK_ATOM)
	neopixelWrite(my.PIN_EXT_RGBLED, (((color >> 16) & 0xFF) >> 1), (((color >> 8) & 0xFF) >> 1), ((color & 0xFF) >> 1));
#endif	//defined(ARDUINO_M5STACK_ATOM)

	return;
}
#endif

#if false	// _my_i2c.hで実装
//******************************************************************************
//	RGB LED設定 for エンコーダーHAT
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//			reg:		レジスター
//			color:		カラーコード
//******************************************************************************
void setEncoderHatRGBLED(MY_I2C_Device* oDevice, uint8_t reg, uint32_t color)
{
	bool bRet = false;

#if	defined(USE_I2C_DEVICE_CLASS)							// Wireクラスを使わず I2C_Class/I2C_Deviceクラスを使う場合
#if defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	if(oDevice->isEnabled())
	{
		// 明るすぎる
		uint8_t buf[3];
		buf[0] = (((color >> 16) & 0xFF) >> 1);
		buf[1] = (((color >> 8) & 0xFF) >> 1);
		buf[2] = ((color & 0xFF) >> 1);
//		bRet = (oDevice->writeBytes(reg, buf, 3) == 3);
		bRet = (oDevice->writeRegister(reg, buf, 3) == 3);
	}
#endif
#endif	//defined(USE_I2C_DEVICE_CLASS)

	return;
}

//******************************************************************************
//	RGB LED設定 for エンコーダーユニット
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//			reg:		レジスター
//			index:		LED No
//			color:		カラーコード
//******************************************************************************
void setEncoderUnitRGBLED(MY_I2C_Device* oDevice, uint8_t reg, uint8_t index, uint32_t color)
{
	bool bRet = false;

#if	defined(USE_I2C_DEVICE_CLASS)							// Wireクラスを使わず I2C_Class/I2C_Deviceクラスを使う場合
	if(oDevice ->isEnabled())
	{
		uint8_t buf[4];
		buf[0] = index;
		buf[1] = (((color >> 16) & 0xFF) >> 1);
		buf[2] = (((color >> 8) & 0xFF) >> 1);
		buf[3] = ((color & 0xFF) >> 1);
		bRet = (oDevice->writeRegister(reg, buf, 4) == 4);
	}
#endif	//defined(USE_I2C_DEVICE_CLASS)

	return;
}
#endif	//false	// _my_i2c.hで実装

#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
//******************************************************************************
//	デュアルボタンユニット関連 for StickC系
//******************************************************************************
static int _pinBlue = -1;						// Buleボタンの GPIOピンNo
static int _pinRed = -1;						// Redボタン GPIOピンNo
//------------------------------------------------------------------------------
//	初期化処理
//		IN	int:	BlueボタンのGPIOピン番号
//			int:	RedボタンのGPIOピン番号
//		OUT:		true = 正常終了, false: エラー
//------------------------------------------------------------------------------
bool db_begin(int pinBlue2, int pinRed2)
{
	bool bRet = false;

	//--------------------------------------------------------------------------
	//	引数で指定されたGPIOピン番号を保存
	//--------------------------------------------------------------------------
	_pinBlue = pinBlue2;
	_pinRed = pinRed2;

	//--------------------------------------------------------------------------
	//	デュアルボタンユニット GPIO初期化
	//--------------------------------------------------------------------------
	pinMode(_pinBlue, INPUT_PULLUP);
	pinMode(_pinRed, INPUT_PULLUP);

	//***** ここまで来れば初期化成功
	bRet = true;

	return(bRet);
}

//--------------------------------------------------------------------------
//	呼び出された時点の Blueボタンの状態取得
//	IN
//	OUT：	true = 押されている。false = 押されていない
//--------------------------------------------------------------------------
bool db_isPushedBlue(void)
{
	return((_pinBlue < 0) ? false : (digitalRead(_pinBlue) == LOW));
}

//--------------------------------------------------------------------------
//	呼び出された時点の Redボタンの状態取得
//	IN
//	OUT：	true = 押されている。false = 押されていない
//--------------------------------------------------------------------------
bool db_isPushedRed(void)
{
	return((_pinRed < 0) ? false : (digitalRead(_pinRed) == LOW));
}
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)

//******************************************************************************
//	タッチパネル処理 for Core2/CoreS3/M5Dial/Tab5
//		IN	eventName:	イベント名ポインター変数へのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procTouchPanel(const char** eventName)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

#if defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3) || defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_TAB5)
	// タッチ情報がなければ何もしない
	if(M5.Touch.getCount() == 0)
	{
		return(mode);
	}

	// 前回状態値保存用
	static uint32_t flickingCnt = 0;
	static uint32_t nextJudgement = 0;

	// タッチ情報を取得
	m5::Touch_Class::touch_detail_t dt = M5.Touch.getDetail();
#ifdef DEBUG_MODE_TOUCH
	M5.Log.printf("- Touch: %s, %s, %s, %s, %s, %s, %s, %s, %s(%d, %d)\n",
		(dt.wasPressed() ? "wasPressed" : "__________"),
		(dt.wasReleased() ? "wasReleased" : "___________"),
		(dt.wasClicked() ? "wasClicked" : "__________"),
		(dt.wasHold() ? "wasHold" : "_______"),
		(dt.wasFlickStart() ? "wasFlickStart" : "_____________"),
		(dt.isFlicking() ? "isFlicking" : "__________"),
		(dt.wasFlicked() ? "wasFlicked" : "__________"),
		(dt.wasDragStart() ? "wasDragStart" : "____________"),
		(dt.wasDragged() ? "wasDragged" : "__________"),
		dt.x, dt.y
	);
#endif	//DEBUG_MODE_TOUCH
	// クリックされた
	if(dt.wasClicked())
	{
		*eventName = "Touch.wasClicked()";
		mode = MY::MEDIA_CONTROL::MUTE;
	}
	// ホールドされていた
	if(dt.wasHold())
	{
		*eventName = "Touch.wasHold()";
		mode = MY::MEDIA_CONTROL::MODE_CHANGE;
	}
	// フリック開始
	if(dt.wasFlickStart())
	{
		flickingCnt = 0;
		nextJudgement = (millis() + my.DELAY_FLICKING_START);
	}
	// フリック中
	else if(dt.isFlicking())
	{
		*eventName = "Touch.Flicking()";
		flickingCnt++;
		// 規定時間以上フリックしている場合
		if(nextJudgement < millis())
		{
			nextJudgement = (millis() + my.DELAY_FLICKING_DURATION);
			// 横方向より縦方向の動きが大きい場合、ボリュームコントロール
			if(abs(dt.distanceX()) < abs(dt.distanceY()))
			{
				// ボリュームアップ（原点が左上なので下方法がプラス）
				if(dt.distanceY() < 0)
				{
					mode = MY::MEDIA_CONTROL::VUP;
				}
				// ボリュームダウン（原点が左上なので上下方法がマイナス）
				else if(0 < dt.distanceY())
				{
					mode = MY::MEDIA_CONTROL::VDOWN;
				}
			}
			// 縦方向より横方向の移動が大きい場合、曲送り
			else
			{
				;
			}
		}
	}
	// フリック終了
	else if(dt.wasFlicked())
	{
//		M5.Log.printf(" : FlickEnd(%d, %d) -> (%d, %d) : (%d, %d), distance(%d, %d)\n", dt.base_x, dt.base_y, dt.x, dt.y, (dt.x - dt.base_x), (dt.y - dt.base_y), dt.distanceX(), dt.distanceY());
		*eventName = "Touch.Flick()";
		// 横方向より縦方向の動きが大きい場合、ボリュームコントロール
		if(abs(dt.distanceX()) < abs(dt.distanceY()))
		{
			// ボリュームアップ（原点が左上なので下方法がプラス）
			if(dt.distanceY() < 0)
			{
				mode = MY::MEDIA_CONTROL::VUP;
			}
			// ボリュームダウン（原点が左上なので上下方法がマイナス）
			else if(0 < dt.distanceY())
			{
				mode = MY::MEDIA_CONTROL::VDOWN;
			}
		}
		// 縦方向より横方向の移動が大きい場合、曲送り
		else
		{
			// 前トラック
			if(dt.distanceX() < 0)
			{
				mode = MY::MEDIA_CONTROL::PREVIOUS;
			}
			// 次トラック
			else if(0 < dt.distanceX())
			{
				mode = MY::MEDIA_CONTROL::NEXT;
			}
		}
	}
#endif	//defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3) || defined(ARDUINO_M5STACK_DIAL)

	return(mode);
}

//******************************************************************************
//	エンコーダー処理 for M5Dial
//		IN:		void
//		OUT:	メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procM5DialEncoder(void)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

#if defined(ARDUINO_M5STACK_DIAL)
	// 前回値保存用
	static uint32_t oldValue = 0;

	//*** エンコーダー値取得
	int32_t newValue = M5Dial.Encoder.read();
#ifdef DEBUG_MODE
//	M5.Log.printf("- M5Dial: oldValue = %d, newValue = %d\n", oldValue, newValue);
#endif	//DEBUG_MODE

	//*** メディアコントロールモード設定
	// ボリュームダウン
	if(newValue < oldValue)
	{
		mode = MY::MEDIA_CONTROL::VDOWN;
	}
	// ボリュームアップ
	else if(oldValue < newValue)
	{
		mode = MY::MEDIA_CONTROL::VUP;
	}
	// 前回値として保存
	oldValue = newValue;
#endif	//defined(ARDUINO_M5STACK_DIAL)

	return(mode);
}

//******************************************************************************
//	ボタン処理 for ALL(Core2/CoreS3/Tab5除く)
//	※ 機種ごとの違いはここで吸収する。
//		IN	_button:	MY::BUTTON識別用列挙体
//			oDevice:	Buttonクラスオブジェクトへのポインター
//			eventName:	イベント名ポインター変数へのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procButton(MY::BUTTON _button, m5::Button_Class* oDevice, const char** eventName)
{
	static const char* btnName[MY::BUTTON::BUTTON_MAX] = { "no button", "BtnA", "BtnB", "BtnC", "BtnEXT", "BtnPWR" };
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

#if defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3) || defined(ARDUINO_M5STACK_TAB5)
	//--------------------------------------------------------------------------
	//	Core2/CoreS3ではボタンエリアもタッチエリアなので、タッチ系イベントも同時に発生してしまう。
	//	そのためボタンは同時に使用しない方がいいと判断したので、何もしないよ。
	//--------------------------------------------------------------------------
#else
	// 以下、Core2/CoreS3/Tab5以外
	// 前回値保存用
	static m5::Button_Class::button_state_t oldSts = m5::Button_Class::button_state_t::state_nochange;

	//*** ボタン状況取得
	m5::Button_Class::button_state_t newSts = oDevice->getState();
	//*** メディアコントロールモード設定
	// 前回値から変化なければ何もしない
	if(newSts == oldSts)
	{
		return(mode);
	}
	// 取得値を前回値として保存
	oldSts = newSts;

#ifdef DEBUG_MODE_BUTTON
	M5.Log.printf("- Button: %s, %s, %s, %s, %s, %s, %s\n",
		(oDevice->wasPressed() ? "wasPressed" : "__________"),
		(oDevice->wasReleased() ? "wasReleased" : "___________"),
		(oDevice->wasReleasedAfterHold() ? "wasReleasedAfterHold" : "____________________"),
		(oDevice->wasClicked() ? "wasClicked" : "__________"),
		(oDevice->isPressed() ? "isPressed" : "_________"),
		(oDevice->isHolding() ? "isHolding" : "_________"),
		(oDevice->wasHold() ? "wasHold" : "_______")
	);
#endif	//DEBUG_MODE_BUTTON

#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	//*** StickC系の場合、BtnAは BtnBと同等に扱う
	if(_button == MY::BUTTON::BUTTON_A)
	{
		_button = MY::BUTTON::BUTTON_B;
	}
#elif defined(ARDUINO_M5STACK_COREINK)
	//*** CORE.INKの場合、BtnAと BtnCの動きは逆にする
	if(_button == MY::BUTTON::BUTTON_A
	{
		_button = MY::BUTTON::BUTTON_C;
	}
	else if(_button == MY::BUTTON::BUTTON_C)
	{
		_button = MY::BUTTON::BUTTON_A;
	}
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)

	//*** (1) ホールドされていた
	if(oDevice->wasHold() == true)
	{
		*eventName = "Btn.wasHold()";
		mode = MY::MEDIA_CONTROL::MODE_CHANGE;				// モード変更
	}
	//*** (2) ホールド後にリリースされた：wasReleasd()に反応しないようにキャッチ
	else if(oDevice->wasReleasedAfterHold())
	{
		*eventName = "Btn.wasReleasedAfterHold()";
#ifdef DEBUG_MODE
		M5.Log.printf("- Btn.wasReleasedAfterHold(): ホールド後のリリースなので何もしない\n");
#endif	//DEBUG_MODE
	}
	// (3) リリースされた：長押し後には wasReleased()と wasReleasedAfterHold()の両方がキャッチできるので注意
	else if(oDevice->wasReleased())
	{
		switch(_button)
		{
			case MY::BUTTON::BUTTON_A:
				*eventName = "BtnA.wasReleased()";
				// 音量ダウン/前のトラック/早戻し
				mode = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? MY::MEDIA_CONTROL::VDOWN : ((my.controlMode == MY::CONTROL_MODE::TRACK) ? MY::MEDIA_CONTROL::PREVIOUS : MY::MEDIA_CONTROL::REWIND));
				break;
			case MY::BUTTON::BUTTON_B:
				*eventName = "BtnB.wasReleased()";
				// ミュート/再生/再生
				mode = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? MY::MEDIA_CONTROL::MUTE : ((my.controlMode == MY::CONTROL_MODE::TRACK) ? MY::MEDIA_CONTROL::PAUSE : MY::MEDIA_CONTROL::PAUSE));
				break;
			case MY::BUTTON::BUTTON_C:
				*eventName = "BtnC.wasReleased()";
				// 音量アップ/次のトラック/早送り
				mode = ((my.controlMode == MY::CONTROL_MODE::VOLUME) ? MY::MEDIA_CONTROL::VUP : ((my.controlMode == MY::CONTROL_MODE::TRACK) ? MY::MEDIA_CONTROL::NEXT : MY::MEDIA_CONTROL::FORWARD));
				break;
		}
	}
#endif	//!defined(ARDUINO_M5STACK_CORE2) && defined(ARDUINO_M5STACK_CORES3)

	return(mode);
}

//******************************************************************************
//	エンコーダーユニット処理 for ALL
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procEncoderUnit(MY_I2C_ENCODER_UNIT* oDevice)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

	// 無効なら何もしない
	if(!oDevice->isEnabled())
	{
		// ※ 初期化時に無効でも動作するっぽいから return()をコメントアウトしてみる
//		return(mode);
	}

	// 前回値保存用
	static int16_t oldValue = 0;
	static bool oldButton = false;
	// 今回値
	int16_t newValue = 0;
	bool newButton = false;

	//*** エンコーダー値取得(絶対位置)
	newValue = oDevice->getValue(oldValue);
	//*** ボタン状態取得: false = 押されてない, true = 押されている
	newButton = oDevice->getButton(oldButton);

	//*** メディアコントロールモード設定
	// ボタンが押されていれば、ミュート処理
	if(newButton)
	{
		// ミュート
		if(oldButton == false)
		{
			mode = MY::MEDIA_CONTROL::MUTE;
		}
	}
	// ボタンが押されていなければ、ボリューム操作
	else
	{
		// ボリュームダウン
		if(newValue < oldValue)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s : old = %d, new = %d\n", oDevice->getName(), oldValue, newValue);
#endif	//DEBUG_MODE
			mode = MY::MEDIA_CONTROL::VDOWN;
		}
		// ボリュームアップ
		else if(oldValue < newValue)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s : old = %d, new = %d\n", oDevice->getName(), oldValue, newValue);
#endif	//DEBUG_MODE
			mode = MY::MEDIA_CONTROL::VUP;
		}
	}
	// 前回値として保存
	oldValue = newValue;
	oldButton = newButton;

	return(mode);
}

//******************************************************************************
//	エンコーダーモジュール処理 for Core(Gray/Fire/Core2/CoreS3用)
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procEncoderModule(MY_I2C_ENCODER_MODULE* oDevice)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
	// 無効なら何もしない
	if(!oDevice->isEnabled())
	{
//		M5.Log.printf("- %s: isEnabled() = %d\n", oDevice->getName(), oDevice->isEnabled());
		return(mode);
	}

	// 前回値保存用
	static int8_t oldValue = 0;
	static bool oldButton = false;
	// 今回値
	uint8_t buf[2];
	int8_t newValue = 0;
	bool newButton = false;

	//*** エンコーダー値取得(絶対位置)
	newValue = oDevice->getValue(oldValue, oldButton);
	//*** ボタン状態取得: false = 押されてない, true = 押されている
	newButton = oDevice->getButton(oldButton);

	// ボタンが押されていれば、ミュート処理
	if(newButton)
	{
		// ミュート
		if(oldButton == false)
		{
			mode = MY::MEDIA_CONTROL::MUTE;
		}
	}
	// ボタンが押されていなければ、ボリューム操作
	else
	{
		// ボリュームダウン
		if(newValue < 0)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s : old = %d, new = %d\n", oDevice->getName(), oldValue, newValue);
#endif	//DEBUG_MODE
			mode = MY::MEDIA_CONTROL::VDOWN;
		}
		// ボリュームアップ
		else if(0 < newValue)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s : old = %d, new = %d\n", oDevice->getName(), oldValue, newValue);
#endif	//DEBUG_MODE
			mode = MY::MEDIA_CONTROL::VUP;
		}
	}
	// 前回値として保存
	oldValue = newValue;
	oldButton = newButton;
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)

	return(mode);
}

//******************************************************************************
//	HMIモジュール処理 for Core(Gray/Fire/Core2/CoreS3用)
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procHmiModule(MY_I2C_HMI_MODULE* oDevice)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;
	uint8_t readData;

#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
	// 無効なら何もしない
	if(!oDevice->isEnabled())
	{
		return(mode);
	}

	// 前回値保存用
	static uint32_t oldValue = 0;
	static bool oldButton = false;
	static bool oldButtonA = false;
	static bool oldButtonB = false;
	// 今回値
	uint8_t buf[4];
	uint32_t newValue = 0;
	bool newButton = false;
	bool newButtonA = false;
	bool newButtonB = false;

	//*** エンコーダー値取得(絶対位置)
	newValue = oDevice->getValue(oldValue);
	//*** ボタン状態取得: false = 押されてない, true = 押されている
	newButton = oDevice->getButton(oldButton);

	//*** ボタンA状態取得: false = 押されてない, true = 押されている
	newButtonA = oDevice->getButton(oldButton, MY_I2C_HMI_MODULE::BUTTON_A);
	//*** ボタンB状態取得: false = 押されてない, true = 押されている
	newButtonB = oDevice->getButton(oldButton, MY_I2C_HMI_MODULE::BUTTON_B);
#ifdef DEBUG_MODE
//	M5.Log.printf("+ read = %d, value = %d ->%d, BtnS = %d -> %d, BtnA = %d, BtnB = %d\n", bRet, oldValue, newValue, oldButton, newButton, newButtonA, newButtonB);
#endif	//DBUGE_MODE

	// ボタンが押されていれば、ミュート処理
	if(newButton)
	{
		// ミュート
		if(oldButton == false)
		{
			mode = MY::MEDIA_CONTROL::MUTE;
		}
	}
	// ボタンが押されていなければ、ボリューム操作
	else
	{
		// ボリュームダウン
		if(newValue < oldValue)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s : old = %d, new = %d\n", oDevice->getName(), oldValue, newValue);
#endif	//DEBUG_MODE
			mode = MY::MEDIA_CONTROL::VDOWN;
		}
		// ボリュームアップ
		else if(oldValue < newValue)
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s : old = %d, new = %d\n", oDevice->getName(), oldValue, newValue);
#endif	//DEBUG_MODE
			mode = MY::MEDIA_CONTROL::VUP;
		}
	}

	// ボタンAが押されていたら
	if(newButtonA && (oldButtonA == false))
	{
		// LED値を取得
		uint8_t ret1 = oDevice->readRegister(MY_I2C_HMI_MODULE::REG_GET_LED_A, buf, 1);
		uint8_t oldLED = buf[0];
		buf[0] = ((oldLED == 0) ? 1 : 0);
		// LED値をセット
		uint8_t ret2 = oDevice->writeRegister(MY_I2C_HMI_MODULE::REG_GET_LED_A, buf, 1);
#ifdef DEBUG_MODE
		M5.Log.printf("+ read = %d, oldLED = %d, write = %d\n", ret1, oldLED, ret2);
#endif	//DBUGE_MODE
	}

	// ボタンBが押されていたら
	if(newButtonB && (oldButtonB == false))
	{
		// LED値を取得
		uint8_t ret1 = oDevice->readRegister(MY_I2C_HMI_MODULE::REG_GET_LED_B, buf, 1);
		uint8_t oldLED = buf[0];
		buf[0] = ((oldLED == 0) ? 1 : 0);
		// LED値をセット
		uint8_t ret2 = oDevice->writeRegister(MY_I2C_HMI_MODULE::REG_GET_LED_B, buf, 1);
#ifdef DEBUG_MODE
		M5.Log.printf("+ read = %d, oldLED = %d, write = %d\n", ret1, oldLED, ret2);
#endif	//DBUGE_MODE
	}
	// 前回値として保存
	oldValue = newValue;
	oldButton = newButton;
	oldButtonA = newButtonA;
	oldButtonB = newButtonB;
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)

	return(mode);
}

//******************************************************************************
//	エンコーダーHAT処理 for StickC
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procEncoderHat(MY_I2C_Device* oDevice)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
#if false
	// 無効なら何もしない
	if(!oDevice->isEnabled())
	{
		return(mode);
	}
#endif

	// 前回値保存用
	static int32_t oldValue = 0;
	static bool oldButton = false;
	// 今回値
	uint8_t buf[4];
	uint32_t newValue = 0;
	bool newButton = 0;

	//*** エンコーダー値取得
	if(oDevice->readRegister(MY_I2C_ENCODER_HAT::REG_GET_VALUE, buf, 4))
	{
		newValue = ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
	}

	//*** ボタン状態取得: 1 = 押されてない, 0 = 押されている
	if(oDevice->readRegister(MY_I2C_ENCODER_HAT::REG_GET_BUTTON, buf, 1))
	{
		newButton = (buf[0] == 0);
	}
	//	M5.Log.printf("- procEncoderHat(): newValue = %d, newButton = %d\n", newValue, newButton);

	// ボタンが押されていれば、ミュート処理
	if(newButton)
	{
		// ミュート
		if(oldButton == false)
		{
			mode = MY::MEDIA_CONTROL::MUTE;
		}
	}
	// ボタンが押されていなければ、ボリューム操作
	else if(newValue != oldValue)
	{
		// ボリューム操作時にはデュアルボタンユニットの Blue/Redボタンの押下状態取得
		bool bBlueButton = db_isPushedBlue();
		bool bRedButton = db_isPushedRed();
#ifdef DEBUG_MODE
		M5.Log.printf("\tBlue Button = %s, Red Button = %s\n", (bBlueButton ? "Pushed" : "Released"), (bRedButton ? "Pushed" : "Released"));
#endif	//DBUGE_MODE
		// ボリュームダウン
		if(newValue < oldValue)
		{
			mode = MY::MEDIA_CONTROL::VDOWN;
		}
		// ボリュームアップ
		else if(oldValue < newValue)
		{
			mode = MY::MEDIA_CONTROL::VUP;
		}
	}
	// 前回値保存
	oldValue = newValue;
	oldButton = newButton;
#endif

	return(mode);
}

//******************************************************************************
//	Mini JoyC HAT処理 for StickC
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procMiniJoycHat(MY_I2C_Device* oDevice)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	// 無効なら何もしない
	if(!oDevice->isEnabled())
	{
		return(mode);
	}

	// 前回値保存用
	static int8_t oldX = 0;
	static int8_t oldY = 0;
	static bool oldButton = false;
	// 今回値
	uint8_t buf[4];
	int8_t newX = 0;
	int8_t newY = 0;
	int8_t newZ = 0;
	uint16_t newXX = 0;
	uint16_t newYY = 0;
	bool newButton = false;

	//*** ジョイスティック値取得: X座標 -128 ← 0 → +127, Y座標 -128 ↓ 0 ↑ +127
	newX = oDevice->readRegister(my.REG_JOYC_HAT_INT8, buf, 2);
	newY = oDevice->readRegister(my.REG_JOYC_HAT_ADC_VAL, buf, 4);
	newZ = oDevice->readRegister(my.REG_JOYC_HAT_BUTTON, buf, 1);
#ifdef DEBUG_MODE
	M5.Log.printf("+ procMiniJoycHat(): REG_JOYC_HAT_INT8 = %dbytes, REG_JOYC_HAT_ADC_VAL = %dbytes, REG_JOYC_HAT_BUTTON = %dbytes\n", newX, newY, newZ);
#endif	//DEBUG_MODE
//	if(oDevice->readRegister(my.REG_JOYC_HAT_INT8, buf, 2) == 2)
	{
		newX = (int8_t)buf[0];
		newY = (int8_t)buf[1];
	}
	//*** ジョイスティック値取得: X座標 0 ～ 4095, Y座標 0 ～ 4095
//	if(oDevice->readRegister(my.REG_JOYC_HAT_ADC_VAL, buf, 4) == 4)
	{
		newXX = ((buf[1] << 8) | buf[0]);
		newYY = ((buf[3] << 8) | buf[2]);
	}
	//*** ボタン押下状態取得: 
//	if(oDevice->readRegister(my.REG_JOYC_HAT_ADC_VAL, buf, 1) == 1)
	{
		newButton = (buf[0] ? true : false);
	}
#ifdef DEBUG_MODE
	M5.Log.printf("+ procMiniJoycHat(): X = %d, Y = %d, Button = %d, XX = %d, YY = %d\n", newX, newY, newButton, newXX, newYY);
#endif	//DEBUG_MODE

	// ボタンが押されていれば、ミュート処理
	if(newButton)
	{
		// ミュート
		if(oldButton == false)
		{
			// MUTE処理
			mode = MY::MEDIA_CONTROL::MUTE;
		}
	}
	// ボタンが押されていなければ、ボリューム操作
	else
	{
#if false
		// ボリュームダウン
		if(newValue < oldValue)
		{
			mode = MY::MEDIA_CONTROL::VDOWN;
		}
		// ボリュームアップ
		else if(oldValue < newValue)
		{
			mode = MY::MEDIA_CONTROL::VUP;
		}
#endif
	}
	// 前回値保存
	oldX = newX;
	oldY = newY;
	oldButton = newButton;
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)

	return(mode);
}

//******************************************************************************
//	ジョイスティックHAT処理 for StickC
//		IN	oDevice:	I2Cデバイスオブジェクトへのポインター
//		OUT:			メディアコントロールモード
//******************************************************************************
MY::MEDIA_CONTROL procJoystickHat(MY_I2C_Device* oDevice)
{
	MY::MEDIA_CONTROL mode = MY::MEDIA_CONTROL::NONE;

#if defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
#if false  // ※※※※※ 古いジョイスティックハットは使えなくいいよね ※※※※※
	// 無効なら何もしない
	if(!oDevice->isEnabled())
	{
		return(mode);
	}

	// 前回値保存用
	static int8_t oldX = 0;
	static int8_t oldY = 0;
	static bool oldButton = false;
	// 今回値
	uint8_t buf[4];
	int8_t newX = 0;
	int8_t newY = 0;
	uint16_t newXX = 0;
	uint16_t newYY = 0;
	bool newButton = false;

	//*** ジョイスティック値取得: X座標 -128 ← 0 → +127, Y座標 -128 ↓ 0 ↑ +127
	//*** ボタン状態取得: 1 = 押されてない, 0 = 押されている
	if(oDevice->readBytes(my.REG_JOYSTICK_HAT_VALUE, buf, 3) == 3)
	{
		newX = (int8_t)buf[0];
		newY = (int8_t)buf[1];
		newButton = (buf[2] == 0);
	}
	//*** ジョイスティック値取得: X座標 0 ～ 4095, Y座標 0 ～ 4095
	if(oDevice->readBytes(my.REG_JOYSTICK_HAT_AXIS, buf, 4) == 4)
	{
		newXX = ((buf[1] << 8) | buf[0]);
		newYY = ((buf[3] << 8) | buf[2]);
	}
#ifdef DEBUG_MODE
//	M5.Log.printf("+ procJoystickHat(): X = %d, Y = %d, Button = %d, XX = %d, YY = %d\n", newX, newY, newButton, newXX, newYY);
//	M5.Log.printf("+ %procJoystickHat): 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", buf[0], buf[1], buf[2], buf[3]);
#endif	//DEBUG_MODE

	// ボタンが押されていれば、ミュート処理
	if(newButton)
	{
		// ミュート
		if(oldButton == false)
		{
			// MUTE処理
			mode = MY::MEDIA_CONTROL::MUTE;
		}
	}
	// ボタンが押されていなければ、ボリューム操作
	else
	{
		// ボリュームダウン
		if(newValue < oldValue)
		{
			mode = MY::MEDIA_CONTROL::VDOWN;
		}
		// ボリュームアップ
		else if(oldValue < newValue)
		{
			mode = MY::MEDIA_CONTROL::VUP;
		}
	}
	// 前回値保存
	oldX = newX;
	oldY = newY;
	oldButton = newButton;
#endif
#endif

	return(mode);
}
