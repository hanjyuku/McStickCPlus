//******************************************************************************
//	I2Cデバイス管理クラス
//		さほど複雑ではないので cppファイルはありません
//------------------------------------------------------------------------------
//		2024/08/16:	ファーストリリース
//		2024/08/17:	init()で初期化に失敗しても _nameのみは保存する
//		2024/08/17: クラス名を MY_I2Cに変更
//		2024/09/03: 全ソース/ヘッダーファイルの同期
//		2024/09/14: MY_I2C_Deviceクラスを新規作成
//		2024/09/14: begin()内 start()の周波数を400000に固定
//		2025/09/22:	I2Cデバイス制御クラスを各デバイスごとに派生クラスを持たせるように変更(エンコーダーユニット、エンコーダーHAT)
//		2025/09/23:	I2Cデバイス関連の設定を各デバイスごとのクラスに移動
//		2026/09/05:	ENV.Ⅲユニット対応
//******************************************************************************
#if !defined(__my_i2c_h__)
#define	__my_i2c_h__

#include "utility/I2C_Class.hpp"

//******************************************************************************
//	M5UnifiedのI2C_Deviceクラスから派生した独自I2Cデバイス管理クラス
//------------------------------------------------------------------------------
//		Wireクラスではなく I2C_Class/I2C_Deviceクラスを使用
//******************************************************************************
class MY_I2C_Device : public m5::I2C_Device
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	static constexpr char* UNIQUE_NAME = "Unknown-Unit";	// デバイス名称	「constexpr」ではなく「const」を指定するとコンパイルエラー
	const char* _name;										// デバイス名称

public:
	//------------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------------
	MY_I2C_Device(std::uint8_t i2c_addr, std::uint32_t freq, m5::I2C_Class* i2c, const char* name) : m5::I2C_Device(i2c_addr, freq, i2c), _name(name){}
	MY_I2C_Device(std::uint8_t i2c_addr, m5::I2C_Class* i2c, const char* name) : m5::I2C_Device(i2c_addr, FREQ, i2c), _name(name){}

	//------------------------------------------------------------------------------
	//	Private/Protectedメンバー変数用Setter/Getter
	//------------------------------------------------------------------------------
	// ※末尾の「const」はオブジェクトの状態を変更しないことを示す
	// from I2C_Deviceクラス
	m5::I2C_Class* getI2C(void)const{return(_i2c);}
	std::uint32_t getFreq(void)const{return(_freq);}
	std::uint8_t getAddr(void)const{return(_addr);}
	// from 自クラス
	const char* getName(void)const{return(_name);}

	//------------------------------------------------------------------------------
	//	Public変数
	//------------------------------------------------------------------------------
	static const uint32_t FREQ = 400000UL;					// I2Cの周波数	この値以外だと ScanIDでデバイスが見つからないことが多い

	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------
	//	デバイス初期化
	//	Privateメンバー変数の _initの設定も行う
	//		IN	void
	//		OUT	bool:	true = 有効, false: 無効
	//------------------------------------------------------
	bool begin(void)
	{
		if((_init = _i2c->start(_addr, false, _freq)))
		{
			_init = _i2c->stop();
#ifdef DEBUG_MODE
			M5.Log.printf("*** MY_I2C_Device::begin(): stop() = %d\n", _init);
#endif	//DEBUG_MODE
		}
		else
		{
#ifdef DEBUG_MODE
			M5.Log.printf("*** MY_I2C_Device::begin(): start() = %d\n", _init);
#endif	//DEBUG_MODE
		}

		return(_init);
	}

	//------------------------------------------------------
	//	レジスター指定しないRead
	//		IN	buf:	バッファーへのポインター
	//			len:	バッファー長
	//		OUT	bool:	true = 有効, false: 無効
	//------------------------------------------------------
	bool readBytes(uint8_t* buf, int len)
	{
		bool bRet = false;

		if((bRet = _i2c->start(_addr, true, _freq)))	//***** readだから第2引数は"true" 
		{
			bRet = _i2c->read(buf, len);
			_i2c->stop();
		}

		return(bRet);
	}

	//------------------------------------------------------
	//	レジスター指定しないWrite
	//		IN	buf:	バッファーへのポインター
	//			len:	バッファー長
	//		OUT	bool:	true = 有効, false: 無効
	//------------------------------------------------------
	bool writeBytes(uint8_t* buf, int len)
	{
		bool bRet = false;

		if((bRet = _i2c->start(_addr, false, _freq)))	//***** writeだから第2引数は"false" 
		{
			bRet = _i2c->write(buf, len);
			bRet = _i2c->stop();
		}

		return(bRet);
	}
};

//******************************************************************************
//	独自I2Cデバイス管理クラスから派生した ENV.Ⅲユニット制御クラス(温湿度センサー SHT30)
//******************************************************************************
class MY_I2C_ENV_UNIT_S : public MY_I2C_Device
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	static const uint8_t UNIQUE_ADDR = 0x44;				// I2Cアドレス
	static const uint32_t UNIQUE_FREQ = 400000UL;			// I2Cの周波数	この値以外だと ScanIDでデバイスが見つからないことが多い;
	static constexpr char* UNIQUE_NAME = "ENV3-Unit_S";		// デバイス名称	「constexpr」ではなく「const」を指定するとコンパイルエラー
	float _newHumidity;										// getValue()呼び出し時にセットされ、getValue2()で返される湿度

public:
	//------------------------------------------------------------------------------
	//	Public変数
	//------------------------------------------------------------------------------

	//------------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------------
	MY_I2C_ENV_UNIT_S(uint8_t addr, uint32_t freq, m5::I2C_Class* i2c, const char* name) : MY_I2C_Device(addr, freq, i2c, name){}
	MY_I2C_ENV_UNIT_S(m5::I2C_Class* i2c) : MY_I2C_ENV_UNIT_S(UNIQUE_ADDR, UNIQUE_FREQ, i2c, UNIQUE_NAME){}

	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	温度取得
	//		IN	void
	//		OUT	float:	取得値
	//------------------------------------------------------------------------------
	float getValue(void)
	{
		bool bRet = false;
		uint8_t buf[6];
		float newValue = 123.45;

		memset(buf, 0x00, sizeof(buf));
		buf[0] = 0x2C;
		buf[1] = 0x06;
		bRet = this->writeBytes(buf, 2);
		if(bRet)
		{
			// write()と read()の間に必要。ただしあまり大きすぎると全体的に処理が遅くなる
//			delay(200);
			delay(15);
			bRet = this->readBytes(buf, 6);										// 温度と湿度(センサーの計測値)
			// 物理量(温度℃、相対湿度%RH)に正規化
			newValue = ((((buf[0] * 256.0) + buf[1]) * 175) / 65535.0) - 45;	// 温度。戻り値として返される。
			_newHumidity = ((((buf[3] * 256.0) + buf[4]) * 100) / 65535.0);		// 湿度。getValue2()の戻り値となる。
		}

		return(newValue);
	}

	//------------------------------------------------------------------------------
	//	湿度取得。getValue()で取得した値を返すだけ
	//		IN	void
	//		OUT	float:	取得値
	//------------------------------------------------------------------------------
	float getValue2(void)
	{
		return(_newHumidity);
	}
};

#if false
//******************************************************************************
//	独自I2Cデバイス管理クラスから派生した ENV.Ⅲユニット制御クラス(気圧センサー QMP6988)
//******************************************************************************
class MY_I2C_ENV_UNIT_Q : public MY_I2C_Device
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	static const uint8_t UNIQUE_ADDR = 0x70;				// I2Cアドレス
	static const uint32_t UNIQUE_FREQ = 400000UL;			// I2Cの周波数	この値以外だと ScanIDでデバイスが見つからないことが多い;
	static constexpr char* UNIQUE_NAME = "ENV3-Unit_Q";		// デバイス名称	「constexpr」ではなく「const」を指定するとコンパイルエラー
	//*** レジスター設定
	static const uint8_t REG_GET_PRESSUERE = 0xFA;			// エンコーダー値取得
	float _newTemperature;									// getValue()呼び出し時にセットされ、getValue2()で返される湿度
	float _newPressure;										// getValue()呼び出し時にセットされ、getValue2()で返される湿度
	float _newAltitude;										// getValue()呼び出し時にセットされ、getValue2()で返される湿度

	//------------------------------------------------------------------------------
	//	Public変数
	//------------------------------------------------------------------------------

	//------------------------------------------------------------------------------
	//	Privateメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	温度取得
	//		IN	void
	//		OUT	float:	取得値
	//------------------------------------------------------------------------------
	getValue(void)
	{
		bool bRet = false;
		float fRet = 0.0f;
		uint8_t err = 0;
		uint32_t P_read, T_read;
		int32_t P_raw, T_raw;
		uint8_t a_data_uint8_tr[6] = {0};
		int32_t T_int, P_int;

		// press
		bRet = this->readRegister(_addr, REG_GET_PRESSUERE, a_data_uint8_tr, 6);
		if(bRet == false)
		{
//			QMP6988_LOG("qmp6988 read press raw error! \r\n");
			return(fRet);
		}

		P_read = (uint32_t)((((uint32_t)(a_data_uint8_tr[0])) << SHIFT_LEFT_16_POSITION) | (((QMP6988_U16_t)(a_data_uint8_tr[1])) << SHIFT_LEFT_8_POSITION) | (a_data_uint8_tr[2]));
		P_raw  = (int32_t)(P_read - SUBTRACTOR);

		T_read = (uint32_t)((((uint32_t)(a_data_uint8_tr[3])) << SHIFT_LEFT_16_POSITION) | (((QMP6988_U16_t)(a_data_uint8_tr[4])) << SHIFT_LEFT_8_POSITION) | (a_data_uint8_tr[5]));
		T_raw  = (int32_t)(T_read - SUBTRACTOR);

		T_int               = convTx02e(&(qmp6988.ik), T_raw);
		P_int               = getPressure02e(&(qmp6988.ik), P_raw, T_int);
		qmp6988.temperature = (float)T_int / 256.0f;
		qmp6988.pressure    = (float)P_int / 16.0f;

		return qmp6988.pressure;
	}

	float getValue(void)
	{
		bool bRet = false;
		uint8_t buf[6];
		float newValue = 123.45;

		memset(buf, 0x00, sizeof(buf));
		buf[0] = 0x2C;
		buf[1] = 0x06;
		bRet = this->writeBytes(buf, 2);
		if(bRet)
		{
			// write()と read()の間に必要
			delay(200);

			bRet = this->readBytes(buf, 6);										// 温度と湿度が取得できる
			newValue = ((((buf[0] * 256.0) + buf[1]) * 175) / 65535.0) - 45;	// 温度。戻り値として返される。
//			_newHumidity = ((((buf[3] * 256.0) + buf[4]) * 100) / 65535.0);		// 湿度。getValue2()の戻り値となる。
		}

		return(newValue);
	}


public:
	//------------------------------------------------------------------------------
	//	Public変数
	//------------------------------------------------------------------------------
	//*** レジスター設定
	static const uint8_t REG_GET_VALUE = 0x10;				// エンコーダー値取得
	static const uint8_t REG_GET_BUTTON = 0x20;				// ボタン状態取得
	static const uint8_t REG_SET_LED = 0x30;				// LEDカラー設定
	static const uint8_t REG_GET_VER = 0xF0;				// バージョン情報等取得
	static const uint8_t LED_ALL = 0;						// LED両方
	static const uint8_t LED_LEFT = 1;						// LED左側
	static const uint8_t LED_RIGHT = 2;						// LED右側

	//------------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------------
	MY_I2C_ENV_UNIT_Q(uint8_t addr, uint32_t freq, m5::I2C_Class* i2c, const char* name) : MY_I2C_Device(addr, freq, i2c, name){}
	MY_I2C_ENV_UNIT_Q(m5::I2C_Class* i2c) : MY_I2C_ENV_UNIT_Q(UNIQUE_ADDR, UNIQUE_FREQ, i2c, UNIQUE_NAME){}

	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	温度取得
	//		IN	void
	//		OUT	float:	取得値
	//------------------------------------------------------------------------------
	float getValue(void)
	{
		bool bRet = false;
		uint8_t buf[6];
		float newValue = 123.45;

		memset(buf, 0x00, sizeof(buf));
		buf[0] = 0x2C;
		buf[1] = 0x06;
		bRet = this->writeBytes(buf, 2);
		if(bRet)
		{
			// write()と read()の間に必要
			delay(200);

			bRet = this->readBytes(buf, 6);										// 温度と湿度が取得できる
			newValue = ((((buf[0] * 256.0) + buf[1]) * 175) / 65535.0) - 45;	// 温度。戻り値として返される。
//			_newHumidity = ((((buf[3] * 256.0) + buf[4]) * 100) / 65535.0);		// 湿度。getValue2()の戻り値となる。
		}

		return(newValue);
	}

	//------------------------------------------------------------------------------
	//	湿度取得。getValue()で取得した値を返すだけ
	//		IN	void
	//		OUT	float:	取得値
	//------------------------------------------------------------------------------
	float getValue2(void)
	{
//		return(_newHumidity);
		return(3.1415);
	}
};
#endif

//******************************************************************************
//	独自I2Cデバイス管理クラスから派生したエンコーダーユニット制御クラス
//******************************************************************************
class MY_I2C_ENCODER_UNIT : public MY_I2C_Device
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	static const uint8_t UNIQUE_ADDR = 0x40;				// I2Cアドレス
	static const uint32_t UNIQUE_FREQ = 400000UL;			// I2Cの周波数	この値以外だと ScanIDでデバイスが見つからないことが多い;
	static constexpr char* UNIQUE_NAME = "Encoder-Unit";	// デバイス名称	「constexpr」ではなく「const」を指定するとコンパイルエラー

public:
	//------------------------------------------------------------------------------
	//	Public変数
	//------------------------------------------------------------------------------
	//*** レジスター設定
	static const uint8_t REG_GET_VALUE = 0x10;				// エンコーダー値取得
	static const uint8_t REG_GET_BUTTON = 0x20;				// ボタン状態取得
	static const uint8_t REG_SET_LED = 0x30;				// LEDカラー設定
	static const uint8_t REG_GET_VER = 0xF0;				// バージョン情報等取得
	static const uint8_t LED_ALL = 0;						// LED両方
	static const uint8_t LED_LEFT = 1;						// LED左側
	static const uint8_t LED_RIGHT = 2;						// LED右側

	//------------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------------
	MY_I2C_ENCODER_UNIT(uint8_t addr, uint32_t freq, m5::I2C_Class* i2c, const char* name) : MY_I2C_Device(addr, freq, i2c, name){}
	MY_I2C_ENCODER_UNIT(m5::I2C_Class* i2c) : MY_I2C_ENCODER_UNIT(UNIQUE_ADDR, UNIQUE_FREQ, i2c, UNIQUE_NAME){}

	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	RGB LED設定 for エンコーダーユニット
	//		IN	index:		LED No
	//			color:		カラーコード
	//------------------------------------------------------------------------------
	void setRGBLED(uint8_t index, uint32_t color)
	{
		bool bRet = false;

#if	defined(USE_I2C_DEVICE_CLASS)							// Wireクラスを使わず I2C_Class/I2C_Deviceクラスを使う場合
		if(_i2c->isEnabled())
		{
			uint8_t buf[4];
			buf[0] = index;
			buf[1] = (((color >> 16) & 0xFF) >> 1);
			buf[2] = (((color >> 8) & 0xFF) >> 1);
			buf[3] = ((color & 0xFF) >> 1);
			bRet = (_i2c->writeRegister(_addr, REG_SET_LED, buf, 4, _freq) == 4);
		}
#endif	//defined(USE_I2C_DEVICE_CLASS)

		return;
	}

	//------------------------------------------------------------------------------
	//	バージョン情報を取得
	//		IN:	void
	//		OUT:	uint8_t	バージョン
	//------------------------------------------------------------------------------
	uint8_t getVer(void)
	{
		uint8_t ver = 0;

		// 無効なら何もしない
		if(!_i2c->isEnabled())
		{
			return(ver);
		}

		// 今回値
		uint8_t buf[16];

		//*** バージョン情報値取得
		if(_i2c->readRegister(_addr, REG_GET_VER, buf, 16, _freq))
		{
#ifdef DEBUG_MODE
			M5.Log.printf("\t%s.getVer(): ", getName());
			for(int ii = 0; ii < (sizeof(buf)/sizeof(buf[0])); ii++)
			{
				M5.Log.printf("0x%02X ", buf[ii]);
			}
			M5.Log.printf("\n");
#endif	//DEBUG_MODE
		}
	
		return(ver);
	}

	//------------------------------------------------------------------------------
	//	エンコーダー値取得
	//		IN	oldValue:	前回値
	//		OUT	uint16_t:	取得値
	//------------------------------------------------------------------------------
	uint16_t getValue(uint16_t oldValue)
	{
		uint16_t newValue = oldValue;
		uint8_t buf[2];

		if(readRegister(REG_GET_VALUE, buf, 2))
		{
			newValue = ((buf[1] << 8) | buf[0]);
		}

		return(newValue);
	}

	//------------------------------------------------------------------------------
	//	ボタン状態取得
	//		IN	oldButton:	前回値
	//		OUT	bool:		取得値
	//------------------------------------------------------------------------------
	bool getButton(bool oldButton)
	{
		bool newButton = oldButton;
		uint8_t buf[1];

		if(readRegister(REG_GET_BUTTON, buf, 1))
		{
			newButton = (buf[0] == 0);
		}

		return(newButton);
	}
};

//******************************************************************************
//	独自I2Cデバイス管理クラスから派生したminiエンコーダーハット制御クラス
//******************************************************************************
class MY_I2C_ENCODER_HAT : public MY_I2C_Device
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	static const uint8_t UNIQUE_ADDR = 0x42;				// I2Cアドレス
	static const uint32_t UNIQUE_FREQ = 400000UL;			// I2Cの周波数	この値以外だと ScanIDでデバイスが見つからないことが多い;
	static constexpr char* UNIQUE_NAME = "Encoder-HAT";		// デバイス名称	「constexpr」ではなく「const」を指定するとコンパイルエラー

public:
	//------------------------------------------------------------------------------
	//	Public変数
	//------------------------------------------------------------------------------
	//*** レジスター設定
	static const uint8_t REG_GET_VALUE = 0x00;				// エンコーダー値取得
	static const uint8_t REG_GET_BUTTON = 0x20;				// ボタン状態取得
	static const uint8_t REG_SET_LED = 0x30;				// LEDカラー設定

	//------------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------------
	MY_I2C_ENCODER_HAT(uint8_t addr, uint32_t freq, m5::I2C_Class* i2c, const char* name) : MY_I2C_Device(addr, freq, i2c, name){}
	MY_I2C_ENCODER_HAT(m5::I2C_Class* i2c) : MY_I2C_ENCODER_HAT(UNIQUE_ADDR, UNIQUE_FREQ, i2c, UNIQUE_NAME){}

	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	RGB LED設定 for エンコーダーHAT
	//		IN	color:		カラーコード
	//------------------------------------------------------------------------------
	//	※ StickC系の時のみロジックは有効になります
	//------------------------------------------------------------------------------
	void setRGBLED(uint32_t color)
	{
		bool bRet = false;

#if	defined(USE_I2C_DEVICE_CLASS)							// Wireクラスを使わず I2C_Class/I2C_Deviceクラスを使う場合
#if defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		if(_i2c->isEnabled())
		{
			// 明るすぎる
			uint8_t buf[3];
			buf[0] = (((color >> 16) & 0xFF) >> 1);
			buf[1] = (((color >> 8) & 0xFF) >> 1);
			buf[2] = ((color & 0xFF) >> 1);
//			bRet = (oDevice->writeBytes(reg, buf, 3) == 3);
			bRet = (_i2c->writeRegister(_addr, REG_SET_LED, buf, 3, _freq) == 3);
		}
#endif
#endif	//defined(USE_I2C_DEVICE_CLASS)

		return;
	}
};

//******************************************************************************
//	独自I2Cデバイス管理クラスから派生したエンコーダーモジュール制御クラス
//******************************************************************************
class MY_I2C_ENCODER_MODULE : public MY_I2C_Device
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	static const uint8_t UNIQUE_ADDR = 0x62;				// I2Cアドレス
	static const uint32_t UNIQUE_FREQ = 400000UL;			// I2Cの周波数	この値以外だと ScanIDでデバイスが見つからないことが多い
	static constexpr char* UNIQUE_NAME = "Encoder-Module";	// デバイス名称	「constexpr」ではなく「const」を指定するとコンパイルエラー
	bool _oldButton;										// getValue()呼び出し時にセットされ、getButton()で参照されるボタン状態
	bool _newButton;										// getValue()呼び出し時にセットされ、getButton()で返されるボタン状態
public:
	//------------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------------
	MY_I2C_ENCODER_MODULE(uint8_t addr, uint32_t freq, m5::I2C_Class* i2c, const char* name) : MY_I2C_Device(addr, freq, i2c, name)
	{
		_oldButton = false;
		_newButton = false;
	}
	MY_I2C_ENCODER_MODULE(m5::I2C_Class* i2c) : MY_I2C_ENCODER_MODULE(UNIQUE_ADDR, UNIQUE_FREQ, i2c, UNIQUE_NAME){}

	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	エンコーダー値取得
	//		IN	oldValue:	前回値
	//			oldButton:	前回値(ボタン状態)
	//		OUT	uint8_t:	取得値
	//------------------------------------------------------------------------------
	//	※ getValue()と getButton()の 2メソッドを用意しているが、getButton()では
	//		getVlaue()で取得したボタン状態を返すのみ。
	//		そのため getButton()は getValue()直後に呼び出し、単独での使用はしないこと
	//------------------------------------------------------------------------------
	uint8_t getValue(uint8_t oldValue, bool oldButton)
	{
		uint8_t newValue = oldValue;
		_oldButton = oldButton;								// getButton()で参照する
		uint8_t buf[2];

		//*** エンコーダー値/ボタン状態取得: 				// エンコーダー値：±両方向の変化量、ボタン：押されてない = -128、押されている = 1
		if(readBytes(buf, 2))
		{
			newValue = buf[0];
			_newButton = (buf[1] == 1);						// 取得したボタン状態は getButton()の戻り値で取得すること
		}

		return(newValue);
	}

	//------------------------------------------------------------------------------
	//	ボタン状態取得
	//		IN	oldButton:	前回値(参照しない)
	//		OUT	bool:		getVlaue()での取得値
	//------------------------------------------------------------------------------
	//	※ getValue()と getButton()の 2メソッドを用意しているが、getButton()では
	//		getVlaue()で取得したボタン状態を返すのみ。
	//		そのため getButton()は getValue()直後に呼び出し、単独での使用はしないこと
	//------------------------------------------------------------------------------
	bool getButton(bool oldButton)
	{
		return(_newButton);
	}
};

//******************************************************************************
//	独自I2Cデバイス管理クラスから派生した HMIモジュール制御クラス
//******************************************************************************
class MY_I2C_HMI_MODULE : public MY_I2C_Device
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	static const uint8_t UNIQUE_ADDR = 0x41;				// I2Cアドレス
	static const uint32_t UNIQUE_FREQ = 400000UL;			// I2Cの周波数	この値以外だと ScanIDでデバイスが見つからないことが多い;
	static constexpr char* UNIQUE_NAME = "HMI-Module";		// デバイス名称	「constexpr」ではなく「const」を指定するとコンパイルエラー

public:
	//------------------------------------------------------------------------------
	//	Public変数
	//------------------------------------------------------------------------------
	//*** レジスター設定
	static const uint8_t REG_GET_VALUE = 0x00;				// エンコーダー値取得/設定
	static const uint8_t REG_GET_INCREMENT = 0x10;			// エンコーダー増加減値取得
	static const uint8_t REG_GET_BUTTON = 0x20;				// ボタンS状態取得
	static const uint8_t REG_GET_BUTTON_A = 0x21;			// ボタンA状態取得
	static const uint8_t REG_GET_BUTTON_B = 0x22;			// ボタンB状態取得
	static const uint8_t REG_GET_LED_A = 0x30;				// LED A値取得/設定
	static const uint8_t REG_GET_LED_B = 0x31;				// LED A値取得/設定
	//*** ボタンタイプ
	enum BUTTON_TYPE
	{
		BUTTON_S = 0,										// ダイヤル部ボタン
		BUTTON_A,											// ボタンA
		BUTTON_B,											// ボタンB
	};

	//------------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------------
	MY_I2C_HMI_MODULE(uint8_t addr, uint32_t freq, m5::I2C_Class* i2c, const char* name) : MY_I2C_Device(addr, freq, i2c, name){}
	MY_I2C_HMI_MODULE(m5::I2C_Class* i2c) : MY_I2C_HMI_MODULE(UNIQUE_ADDR, UNIQUE_FREQ, i2c, UNIQUE_NAME){}

	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	エンコーダー値取得
	//		IN	oldValue:	前回値
	//		OUT	uint32_t:	取得値
	//------------------------------------------------------------------------------
	uint32_t getValue(uint32_t oldValue)
	{
		uint32_t newValue = oldValue;
		uint8_t buf[4];

		if(readRegister(REG_GET_VALUE, buf, 4))
		{
			newValue = ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
		}

		return(newValue);
	}

	//------------------------------------------------------------------------------
	//	ボタン状態取得(ダイヤル部ボタン)
	//		IN	oldButton:	前回値
	//		OUT	bool:		取得値
	//------------------------------------------------------------------------------
	bool getButton(bool oldButton)
	{
		return(getButton(oldButton, BUTTON_S));
	}
	//------------------------------------------------------------------------------
	//	ボタン状態取得(ボタンタイプ指定)
	//		IN	oldButton:	前回値
	//			buttonType:	ボタンタイプ
	//		OUT	bool:		取得値
	//------------------------------------------------------------------------------
	bool getButton(bool oldButton, BUTTON_TYPE buttonType)
	{
		bool newButton = oldButton;
		uint8_t reg;
		uint8_t buf[1];

		reg = ((buttonType == BUTTON_A) ? REG_GET_BUTTON_A : ((buttonType == BUTTON_B) ? REG_GET_BUTTON_B : REG_GET_BUTTON));
		if((readRegister(reg, buf, 1)))
		{
			newButton = (buf[0] == 0);
		}

		return(newButton);
	}
};

//*** 事前に Wire.hがインクルードされている時のみ有効
#if defined(TwoWire_h)
//******************************************************************************
//	I2Cデバイス管理クラス
//------------------------------------------------------------------------------
//		I2C_Class/I2C_Deviceクラスではなく Wireクラスを使用
//******************************************************************************
class MY_I2C
{
private:
	//------------------------------------------------------------------------------
	//	Private変数
	//------------------------------------------------------------------------------
	bool _enable = false;									// デバイス有効/無効フラグ
	TwoWire* _wire = NULL;									// 接続先Wireインスタンスへのポインター
	uint8_t _addr = 0x00;									// I2Cアドレス
	char* _name = "Unknown";								// デバイス名へのポインター

public:
	//------------------------------------------------------------------------------
	//	Publicメソッド
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//	コンストラクタ/デストラクタ
	//------------------------------------------------------------------------------
	MY_I2C(TwoWire* oWire, uint8_t addr, char* name)
	{
		_enable = init(oWire, addr, name);
	}

	//------------------------------------------------------------------------------
	//	Privateメンバー変数用Setter/Getter
	//------------------------------------------------------------------------------
	// ※末尾の「const」はオブジェクトの状態を変更しないことを示す
	bool isEnabled(void) const{return(_enable);}
	TwoWire* getWire(void) const{return(_wire);}
	uint8_t getAddr(void) const{return(_addr);}
	const char* getName(void) const{return(_name);}

	//------------------------------------------------------
	//	デバイス初期化
	//		IN	oWire:	Wireオブジェクトへのポインター
	//			addr:	I2Cアドレス
	//		OUT	bool:	true = 正常終了, false: エラー
	//------------------------------------------------------
	bool init(TwoWire* oWire, uint8_t addr, char* name)
	{
		_name = name;
		// デバイスの接続チェック
		oWire->beginTransmission(addr);
		if(oWire->endTransmission(true))
		{
			return(false);
		}
		// 接続情報保存
		_enable = true;
		_wire = oWire;
		_addr = addr;

		return(true);
	}

	//------------------------------------------------------
	//	指定されたバイト数Read
	//		IN	buf:	バッファーへのポインタ
	//			len:	リクエストバイト数
	//		OUT	int8_t:	Readできたバイト数
	//------------------------------------------------------
	int8_t readBytes(uint8_t* buf, uint8_t len)
	{
		int8_t readCnt = 0;

		//*** データ要求
		readCnt = _wire->requestFrom(_addr, len);
		for(int ii = 0; ii < readCnt; ii++)
		{
			*(buf + ii) = _wire->read();
		}

		return(readCnt);
	}

	//------------------------------------------------------
	//	レジスターを指定してのRead
	//		IN	reg:	レジスター
	//			buf:	バッファーへのポインタ
	//			len:	リクエストバイト数
	//		OUT	int8_t:	Readできたバイト数
	//------------------------------------------------------
	int8_t readBytes(uint8_t reg, uint8_t* buf, uint8_t len)
	{
		//*** レジスター指定
		_wire->beginTransmission(_addr);
		if(_wire->write(reg) == 0)
		{
			return(0);
		}
		if(_wire->endTransmission(false))
		{
			return(0);
		}

		//*** データ要求
		return(readBytes(buf, len));
	}

	//------------------------------------------------------
	//	レジスターを指定してのWrite
	//		IN	reg:	レジスター
	//			buf:	バッファーへのポインタ
	//			len:	リクエストバイト数
	//		OUT	int8_t:	Writeできたバイト数
	//------------------------------------------------------
	int8_t writeBytes(int8_t reg, uint8_t* buf, uint8_t len)
	{
		int8_t writeCnt = 0;

		//*** レジスター指定
		_wire->beginTransmission(_addr);
		if(_wire->write(reg) == 0)
		{
			return(0);
		}
		//*** データWrite
		for(int ii = 0; ii < len; ii++)
		{
			if(_wire->write(*(buf + ii)) == 0)
			{
				break;
			}
			writeCnt++;
		}
		if(_wire->endTransmission(true))
		{
			return(0);
		}

		return(writeCnt);
	}
};
#endif	//defined(TwoWire_h)
#endif	//!defined(__my_i2c_h__)
