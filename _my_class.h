//******************************************************************************
//	独自変数管理クラス
//------------------------------------------------------------------------------
//		2024/08/25:	inoファイルから分離
//		2024/09/01: ESP-NOW関連追加
//		2024/09/02: カラー設定追加
//		2024/09/03: 全ソース/ヘッダーファイルの同期
//		2024/09/08: ESP-NOW通信関連を _my_espnow.h, _my_ESPNOW.CPPでクラス化
//		2024/09/13: MY_I2Cクラスを MY_I2C_Deviceクラスに置換
//		2024/09/15: I2Cデバイス名配列を追加
//		2024/09/15: Mini OLEDユニット対応
//		2024/09/17: CoreS3対応
//		2024/09/18: StickC Plus2対応
//		2025/08/07: CORE.INK対応
//		2025/09/11:	画面モード列挙体追加
//		2025/09/22:	I2Cデバイス制御クラスを各デバイスごとに派生クラスを持たせるように変更
//		2025/09/23:	I2Cデバイス関連の設定を各デバイスごとのクラスに移動
//		2026/09/05:	ENV.Ⅲユニット対応
//******************************************************************************
#if !defined(__my_class_h__)
#define	__my_class_h__

//*** 画面表示モード
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)  || defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_TAB5)
#define DISP_MODE_CORE										// Core系、Dial、Tab5
#elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
#define DISP_MODE_STICKC									// StickC系
#elif defined(ARDUINO_M5STACK_ATOMS3)
#define DISP_MODE_ATOMS3									// AtomS3
#elif defined(ARDUINO_M5STACK_COREINK)
#define DISP_MODE_COREINK									// CORE.INK
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)  || defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_TAB5)

class MY
{
public:
	//*** 固定値
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE)
	// CORE1固有値
	const int PIN_EXT_RX2 = 16;								// Serial2設定時の RX(M5ボトムも)
	const int PIN_EXT_TX2 = 17;								// Seriale設定時の TX(M5ボトムも)
	const gpio_num_t PIN_EXT_RGBLED = GPIO_NUM_15;			// M5ボトムの LEDバーのピンNo
	const uint16_t LED_EXT_CNT = 10;						// M5ボトムの LEDバーの LED数
	const int PIN_BLUE_BUTTON = 36;							// Port-Bにデュアルボタンユニットを接続した場合の BlueボタンのピンNo(外側の白)
	const int PIN_RED_BUTTON = 26;							// Port-Bにデュアルボタンユニットを接続した場合の RedボタンのピンNo(内側の黄色)
#elif defined(ARDUINO_M5STACK_CORE2)
	// Core2固有値
	const int PIN_EXT_RX2 = 13;								// Serial2設定時の RX(M5ボトム2)
	const int PIN_EXT_TX2 = 14;								// Seriale設定時の TX(M5ボトム2)
	const gpio_num_t PIN_EXT_RGBLED = GPIO_NUM_25;			// M5ボトム2の LEDバーのピンNo
	const uint16_t LED_EXT_CNT = 10;						// M5ボトム2の LEDバーの LED数
	const int PIN_BLUE_BUTTON = 36;							// Port-Bにデュアルボタンユニットを接続した場合の BlueボタンのピンNo(外側の白)
	const int PIN_RED_BUTTON = 26;							// Port-Bにデュアルボタンユニットを接続した場合の RedボタンのピンNo(内側の黄色)
#elif defined(ARDUINO_M5STACK_CORES3)
	// Core3固有値
	const int PIN_EXT_RX2 = 18;								// Serial2設定時の RX(M5ボトム2)
	const int PIN_EXT_TX2 = 17;								// Seriale設定時の TX(M5ボトム2)
	const gpio_num_t PIN_EXT_RGBLED = GPIO_NUM_5;			// M5ボトム3の LEDバーのピンNo
	const uint16_t LED_EXT_CNT = 10;						// M5ボトム3の LEDバーの LED数
	const int PIN_BLUE_BUTTON = 8;							// Port-Bにデュアルボタンユニットを接続した場合の BlueボタンのピンNo(外側の白)
	const int PIN_RED_BUTTON = 9;							// Port-Bにデュアルボタンユニットを接続した場合の RedボタンのピンNo(内側の黄色)
#elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	// StickC系固有値
	const int PIN_EXT_SDA = 0;								// Ext.8P I2C設定時の SDA
	const int PIN_EXT_SCL = 26;								// Ext.8P I2C設定時の SCL
	const int PIN_EXT_RX2 = 0;								// Serial2設定時の RX(Port-A)
	const int PIN_EXT_TX2 = 26;								// Serial2設定時の TX(Port-A)
	const int PIN_BLUE_BUTTON = 33;							// HY2.0-4Pにデュアルボタンユニットを接続した場合の BlueボタンのピンNo(外側の白)
	const int PIN_RED_BUTTON = 32;							// HY2.0-4Pにデュアルボタンユニットを接続した場合の RedボタンのピンNo(内側の黄色)
#elif defined(ARDUINO_M5STACK_ATOM)
	// Atom固有値
	const int PIN_EXT_SDA = 25;								// Ext.4P I2C設定時の SDA
	const int PIN_EXT_SCL = 27;								// Ext.4P I2C設定時の SCL
	const int PIN_EXT_RX2 = 22;								// Ext.5P Seriale設定時の RX
	const int PIN_EXT_TX2 = 19;								// Ext.5P Seriale設定時の TX
	const gpio_num_t PIN_EXT_RGBLED = GPIO_NUM_27;			// RGB LEDのピンNo
	const int LED_EXT_CNT = 1;								// RGB LEDの LED数
#elif defined(ARDUINO_M5STACK_ATOMS3)
	// AtomS3固有値
	const int PIN_EXT_SDA = 38;								// Ext.4P I2C設定時の SDA
	const int PIN_EXT_SCL = 39;								// Ext.4P I2C設定時の SCL
	const int PIN_EXT_RX2 = 5;								// Ext.5P Serial2設定時の RX
	const int PIN_EXT_TX2 = 6;								// Ext.5P Seriale設定時の TX
	const gpio_num_t PIN_RGB_LED = GPIO_NUM_35;				// RGB LEDのピンNo(AtomS3 Uのみ)
	const int LED_EXT_CNT = 1;								// RGB LEDの LED数(AtomS3 Uのみ)
#elif defined(ARDUINO_M5STACK_DIAL)
	// M5Dial固有値
	const int PIN_BLUE_BUTTON = 1;							// Port-Bにデュアルボタンユニットを接続した場合の BlueボタンのピンNo(外側の白)
	const int PIN_RED_BUTTON = 2;							// Port-Bにデュアルボタンユニットを接続した場合の RedボタンのピンNo(内側の黄色)
	const int PIN_EXT_RX2 = 53;								// Port-Aの RX(ビルド通すための暫定設定)
	const int PIN_EXT_TX2 = 54;								// Port-Aの TX(ビルド通すための暫定設定)
#elif defined(ARDUINO_M5STACK_TAB5)
	// Tab5固有値
	const int PIN_EXT_RX2 = 53;								// Port-Aの RX(ビルド通すための暫定設定)
	const int PIN_EXT_TX2 = 54;								// Port-Aの TX(ビルド通すための暫定設定)
#elif defined(ARDUINO_M5STACK_COREINK)
	// CORE.INK固有値
	const int PIN_EXT_RX2 = 53;								// Port-Aの RX(ビルド通すための暫定設定)
	const int PIN_EXT_TX2 = 54;								// Port-Aの TX(ビルド通すための暫定設定)
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE)

	// 外部I2C
	const uint8_t I2C_ADDR_HAT_JOYSTICK = 0x38;				// ジョイスティックハット
	const uint8_t I2C_ADDR_UNIT_MINI_OLED = 0x3C;			// Mini OLEDユニット
	const uint8_t I2C_ADDR_UNIT_ENCODER = 0x40;				// エンコーダー/スクロールユニット
	const uint8_t I2C_ADDR_HAT_ENCODER = 0x42;				// エンコーダーハット
	const uint8_t I2C_ADDR_HAT_JOYC = 0x54;					// Mini JoyCハット
	const uint8_t I2C_ADDR_UNIT_ULTRASONIC = 0x57;			// UltraSonicユニット
	const uint8_t I2C_ADDR_UNIT_IMU = 0x68;					// IMU(MPC6886)ユニット
	// 内部I2C
	const uint8_t I2C_ADDR_COMPASS = 0x10;					// 内蔵コンパス
	const uint8_t I2C_ADDR_RFID = 0x28;						// RFID
	const uint8_t I2C_ADDR_PMIC = 0x34;						// PMIC(Core2/Core3/StickC/Dial)
	const uint8_t I2C_ADDR_AMP = 0x36;						// アンプ
	const uint8_t I2C_ADDR_TOUCH = 0x38;					// タッチパネルコントローラー(FT3267)
	const uint8_t I2C_ADDR_MIC = 0x40;						// 内蔵マイク(CoreS3)
	const uint8_t I2C_ADDR_MODULE_HMI = 0x41;				// HMIモジュール
	const uint8_t I2C_ADDR_RTC = 0x51;						// RTC
	const uint8_t I2C_ADDR_GPIO_EXPANDER = 0x58;			// GPIO EXPANDER(CoreS3)
	const uint8_t I2C_ADDR_MODULE_ENCODER = 0x62;			// エンコーダーモジュール
	const uint8_t I2C_ADDR_IMU = 0x68;						// IMU(MPC6886)
	const uint8_t I2C_ADDR_GYRO = 0x69;						// ジャイロ
	const uint8_t I2C_ADDR_PMIC_CORE1 = 0x75;				// PMIC(Core1)
	// ログ出力用Serial
	const int SERIAL_SPEED = 115200;
	// 機種間Serial2通信
	const int SERIAL2_SPEED = 9600;
	const uint32_t SERIAL2_SETTING = SERIAL_8N1;
	// delay値
	const uint32_t DELAY_LOOP = 100;						// loop()での deray値
	const uint32_t DELAY_MUTE_DURATION = 100;				// ミュート時のブザー音間隔
	const uint32_t DELAY_WIFI = 500;						// WiFi/NTPサーバー接続リトライ時
	const uint32_t RETRY_WIFI = 50;							// WiFi/NTPサーバー接続リトライ回数
	const uint32_t DELAY_FLICKING_START = 300;				// 連続フリック判定秒数
	const uint32_t DELAY_FLICKING_DURATION = 100;			// 連続フリック判定間隔
	const uint32_t DELAY_ESPNOW_TIMEOUT = 10000;			// ESP-NOW通信での応答待ち時間
	const uint32_t DELAY_NEOPIXEL = 50;
	// NTP設定
	const char* NTP_TIMEZONE = "UTC-9";
	const char* NTP_SERVER1 = "ntp.nict.jp";
	const char* NTP_SERVER2 = "time.google.com";
	const char* NTP_SERVER3 = "ntp.jst.mfeed.ad.jp";
	// 曜日
	const char* dow[7] = { "Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat" };
	// エンコーダーユニットの設定は MY_I2C_ENCODER_UNITクラスで
	// エンコーダーモジュールの設定は MY_I2C_ENCODER_UNITクラスで
	// HMIモジュールの設定は MY_I2C_ENCODER_UNITクラスで
	// エンコーダーHATの設定は MY_I2C_ENCODER_HATクラスで
	// JoyStick HAT for Stick
	const uint8_t I2C_JOYSTICK_HAT = I2C_ADDR_HAT_JOYSTICK;	// I2Cアドレス
	const uint8_t REG_JOYSTICK_HAT_AXIS = 0x01;				// REG: X/Y軸の角度 4bytes(0: X low bits, 1: X high bits, 2: Y low bits, 3: Y high bits)
	const uint8_t REG_JOYSTICK_HAT_VALUE = 0x02;			// REG: X/Y軸の増加分値とボタン状態 3bytes(0: X, 1: Y, 2: Button(0: press, 1: re;ease))
	// MiniJoyC HAT for Stick
	const uint8_t I2C_MINIJOYC_HAT = I2C_ADDR_HAT_JOYC;		// I2Cアドレス
	const uint8_t REG_JOYC_HAT_ADC_VAL = 0x00;				// REG: ADC値の取得 4bytes：それぞれ 0～4095(0: X low bits, 1: X high bits, 2: Y low bits, 3: Y high bits)
	const uint8_t REG_JOYC_HAT_INT10 = 0x10;				// REG: int10値の取得 4bytes：それぞれ -512～511(0: X low bits, 1: X high bits, 2: Y low bits, 3: Y high bits)
	const uint8_t REG_JOYC_HAT_INT8 = 0x20;					// REG: int8値の取得 2bytes：それぞれ -128～127(0: X low bits, 1: Y low bits)
	const uint8_t REG_JOYC_HAT_BUTTON = 0x30;				// REG: ボタン状態取得
	const uint8_t REG_JOYC_HAT_RGBLED = 0x40;				// REG: RGB LED状態取得/設定

	// フォント for Core/Dial
	const lgfx::IFont* FONT_CALENDER = &(fonts::AsciiFont8x16);	// カレンダー部フォント：16pxの AsciiFont8x16
	const double FONTSIZE_CALENDER = 1.0;					// カレンダー部フォント：サイズ 1倍 = 16px
	const lgfx::IFont* FONT_CLOCK = &(fonts::Font7);		// 時計フォント：48pxの7セグ風
	const double FONTSIZE_HHMM = 1.25;						// 時計時分部フォント：サイズ 1.25倍 = 60px
	const double FONTSIZE_SS = 0.5;							// 時計秒部フォント：サイズ 0.5倍 = 24px
	// フォント for AtomS3
	const double FONTSIZE_CALENDER_ATOMS3 = 1.0;			// カレンダー部フォント：サイズ 1倍 = 16px
	const double FONTSIZE_HHMM_ATOMS3 = 0.75;				// 時計時分部フォント：サイズ 0.75倍 = 36px
	const double FONTSIZE_SS_ATOMS3 = 0.25;					// 時計秒部フォント：サイズ 0.25倍 = 12px
	// 画面カラー：基本8色
	const int COLOR_BLACK = TFT_BLACK;						// 黒
	const int COLOR_BLUE = TFT_BLUE;						// 青
	const int COLOR_RED = TFT_RED;							// 赤
	const int COLOR_CYAN = TFT_CYAN;						// 水
	const int COLOR_GREEN = TFT_GREEN;						// 緑
	const int COLOR_MAGENTA = TFT_MAGENTA;					// 紫
	const int COLOR_YELLOW = TFT_YELLOW;					// 黄
	const int COLOR_WHITE = TFT_WHITE;						// 白
	// 画面カラー：中間色
	const int COLOR_NAVY = TFT_NAVY;						// 暗い青
	const int COLOR_DARKGREEN = TFT_DARKGREEN;				// 暗い緑
	const int COLOR_DARKCYAN = TFT_DARKCYAN;				// 暗い水
	const int COLOR_MAROON = TFT_MAROON;					// 暗い赤
	const int COLOR_PURPLE = TFT_PURPLE;					// 暗い紫
	const int COLOR_OLIVE = TFT_OLIVE;						// 暗い黄
	const int COLOR_LIGHTGREY = TFT_LIGHTGREY;				// 明るい灰
	const int COLOR_DARKGREY = TFT_DARKGREY;				// 暗い灰
	const int COLOR_ORANGE = TFT_ORANGE;					// 橙
	const int COLOR_GREENYELLOW = TFT_GREENYELLOW;			// 黄緑
	const int COLOR_PINK = TFT_PINK;						// 桃
	const int COLOR_BROWN = TFT_BROWN;						// 茶
	const int COLOR_GOLD = TFT_GOLD;						// 金
	// 画面カラー：
	const int COLOR_FONT_INIT = COLOR_WHITE;				// 初期文字色
	const int COLOR_BACK_INIT = COLOR_BLACK;				// 初期背景色
	const int COLOR_BACK_MAIN = COLOR_LIGHTGREY;			// 通常背景色
	const int COLOR_FONT_CLOCK = COLOR_BLACK;				// 時計の文字色
	const int COLOR_DRAW_INIT = COLOR_LIGHTGREY;			// 初期枠内の色
	const int COLOR_LINE_INIT = COLOR_DARKGREY;				// 初期枠の色
	const int COLOR_DRAW_SECOND = COLOR_DARKGREY;			// 秒の文字色
	const int COLOR_LINE_SECOND = COLOR_LIGHTGREY;			// 秒の枠色
	const int COLOR_MODE_INIT = COLOR_YELLOW;				// Slave/Contorollerモード未確定
	const int COLOR_MODE_SLAVE = COLOR_RED;					// Slave/Contorollerモード: Slave
	const int COLOR_MODE_CONTROLLER = COLOR_BLUE;			// Slave/Contorollerモード: Controller
	// 座標系(基準値) for Core/Dial
	const int32_t CORRECTION_RADIUS = -90;					// 描画角度の補正値
	const uint32_t SCREEN_WIDTH = 240;						// 画面：幅
	const uint32_t SCREEN_HEIGHT = 240;						// 画面：高さ
	const uint32_t R_RADIUS = (SCREEN_HEIGHT / 2);			// 画面の半径
	const uint32_t DIFF_RADIUS2 = 7;						// 画面と描画領域の差分
	const uint32_t R_RADIUS2 = (R_RADIUS - DIFF_RADIUS2);	// 描画領域の半径
	const uint32_t DIFF_RADIUS3 = 20;						// 画面と描画領域の差分
	const uint32_t R_RADIUS3 = (R_RADIUS - DIFF_RADIUS3);	// 描画領域の半径
	const uint32_t X_CENTER = (R_RADIUS - 0);				// 円の中心：X
	const uint32_t Y_CENTER = (R_RADIUS - 0);				// 円の中心：Y
	const uint32_t H_AREA1 = R_RADIUS;						// 表示エリア1：高さ
	const uint32_t H_AREA2 = (R_RADIUS / 4);				// 表示エリア2：高さ
	const uint32_t H_AREA3 = (R_RADIUS / 4);				// 表示エリア3：高さ
	// 座標系(基準値) for AtomS3
	const uint32_t SCREEN_WIDTH_ATOMS3 = 128;				// 画面：幅
	const uint32_t SCREEN_HEIGHT_ATOMS3 = 128;				// 画面：高さ
	const uint32_t H_AREA1_ATOMS3 = (SCREEN_HEIGHT_ATOMS3 / 3);	// 表示エリア1：高さ
	const uint32_t H_AREA2_ATOMS3 = (SCREEN_HEIGHT_ATOMS3 / 3);	// 表示エリア2：高さ
	const uint32_t H_AREA3_ATOMS3 = (SCREEN_HEIGHT_ATOMS3 / 3);	// 表示エリア3：高さ
	// ブザー周波数(Hz)
	// CoreS3SEで周波数が低いほど音が割れるほど大きくなる？？？
//	const float TONE_FREQ_PREVIOUS = 8000;					// 前のトラック
	const float TONE_FREQ_PREVIOUS = 4000;					// 前のトラック
//	const float TONE_FREQ_NEXT = 4000;						// 次のトラック
	const float TONE_FREQ_NEXT = 6000;						// 次のトラック
//	const float TONE_FREQ_PAUSE = 2000;						// 再生・一時停止
	const float TONE_FREQ_PAUSE = 2000;						// 再生・一時停止
	const float TONE_FREQ_VDOWN = 8000;						// ボリュームダウン
//	const float TONE_FREQ_VUP = 4000;						// ボリュームアップ
	const float TONE_FREQ_VUP = 5000;						// ボリュームアップ
	const float TONE_FREQ_MUTE1 = 3000;						// ミュート1
	const float TONE_FREQ_MUTE2 = 4000;						// ミュート2
	const float TONE_FREQ_MODE_CHANGE = 8000;				// モード変更
	const uint32_t TONE_DURATION = 20;						// 再生時間(ms)
	// RGB_LEDカラー for Atom/エンコーダーユニット/エンコーダーHAT
	const uint32_t LEDCOLOR_OFF = 0x000000;					// オフ
	const uint32_t LEDCOLOR_BLACK = 0x000000;				// 黒
	const uint32_t LEDCOLOR_BLUE = 0x000011;				// 青
	const uint32_t LEDCOLOR_RED = 0x110000;					// 赤
	const uint32_t LEDCOLOR_MAGENTA = 0x110011;				// 紫
	const uint32_t LEDCOLOR_GREEN = 0x001100;				// 緑
	const uint32_t LEDCOLOR_CYAN = 0x001111;				// 水
	const uint32_t LEDCOLOR_YELLOW = 0x111100;				// 黄
	const uint32_t LEDCOLOR_WHITE = 0x111111;				// 白
	const uint32_t LEDCOLOR_HI_BLUE = 0x220000;				// まぶしい青
	const uint32_t LEDCOLOR_HI_RED = 0x220000;				// まぶしい赤
	const uint32_t LEDCOLOR_HI_MAGENTA = 0x220022;			// まぶしい紫
	const uint32_t LEDCOLOR_HI_GREEN = 0x002200;			// まぶしい緑
	const uint32_t LEDCOLOR_HI_YELLOW = 0x222200;			// まぶしい黄
	const uint32_t LEDCOLOR_HI_WHITE = 0x222222;			// まぶしい白
	// RGB_LEDカラー for AtomS3U
	const int LEDCOLOR_INITIALIZING = LEDCOLOR_YELLOW;		// 初期化中
	const int LEDCOLOR_I2C_INITIALIZING = LEDCOLOR_RED;		// 初期化中(I2C)
	const int LEDCOLOR_USB_INITIALIZING = LEDCOLOR_CYAN;	// 初期化中(USB)
	const int LEDCOLOR_WIFI_INITIALIZING = LEDCOLOR_MAGENTA;// 初期化中(WiFi)
	const int LEDCOLOR_ESP_INITIALIZING = LEDCOLOR_RED;		// 初期化中(ESP)
	const int LEDCOLOR_WAITING_0 = LEDCOLOR_OFF;			// 待機中(消灯状態)
	const int LEDCOLOR_WAITING_1 = LEDCOLOR_BLUE;			// 待機中(点灯状態)
	const int LEDCOLOR_DOWN_PREVIOUS = LEDCOLOR_RED;		// コントロール受付：ボリュームダウン/前のトラック
	const int LEDCOLOR_UP_NEXT = LEDCOLOR_GREEN;			// コントロール受付：ボリュームアップ/次のトラック
	const int LEDCOLORPAUSE = LEDCOLOR_YELLOW;				// コントロール受付：再生・一時停止
	// RGB_LEDカラー：メディアコントロールモード
	const int LEDCOLOR_NORMAL = 0x000080;					// 待機：青
	const int LEDCOLOR_PREVIOUS = 0x110000;					// 前のトラック：薄い赤
	const int LEDCOLOR_NEXT = 0x001100;						// 次のトラック：薄い緑
	const int LEDCOLOR_PAUSE = 0x001180;					// 再生/一時停止：薄いシアン
	const int LEDCOLOR_VDOWN = 0x110000;					// ボリュームダウン：薄い赤
	const int LEDCOLOR_VUP = 0x001100;						// ボリュームアップ：薄い緑
	const int LEDCOLOR_MUTE = 0x110011;						// ミュート：薄いマゼンタ

	const char* i2cDeviceName[128] = {						// I2Cデバイス名
/*0?*/	".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".",
/*1?*/	"BMM150", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".",
/*2?*/	".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".",
/*3?*/	".", ".", ".", ".", "PMIC(Core2/StickC)", ".", ".", ".", "Touch Panel(Core2)/Joystick HAT", ".", ".", ".", "Mini OLED UNIT", ".", ".", ".",
/*4?*/	"Encoder UNIN", "HMI MODULE", "Encoder HAT", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".",
/*5?*/	".", "RTC(Core2/StickC)", ".", ".", ".", ".", "UltraSonic MODULE", ".", ".", ".", ".", ".", ".", ".", ".", ".",
/*6?*/	".", ".", "Encoder MODULE", ".", ".", ".", ".", "MPU6886", ".", ".", ".", ".", ".", ".", ".", ".",
/*7?*/	".", ".", ".", ".", ".", "PMIC(Core1)", ".", ".", ".", ".", ".", ".", ".", ".", ".", "."
	};

	//*** デバイス間通信コマンドコード列挙型: ESP-NOW通信で uint8_t[]のバッファに収まるように注意！！
	enum DEVICE_CONTROL
	{
		INIT = 0,
		REQUEST_MAC = 0x10,									// MACアドレス要求
		SEND_MAC,											// MACアドレス通知
		REQUEST_NAME = 0x20,								// ニックネーム要求
		SEND_NAME,											// ニックネーム通知
		SEND_KEYCODE = 0x30,								// キーコード送信
		SEND_MOUSE = 0x40,									// マウスコントロールコード送信
		SEND_JOYPAD = 0x50,									// ジョイパッドコントロールコード送信
	};

	//*** メディアコントロールモード列挙型: ESP-NOW通信で uint8_t[]のバッファに収まるように注意！！
	enum MEDIA_CONTROL
	{
		NONE = 0,											// 
		NORMAL,												// 
		PREVIOUS = 0x11,									// 前のトラック
		NEXT,												// 次のトラック
		PAUSE,												// 再生/一時停止
		VDOWN = 0x21,										// 音量ダウン
		VUP,												// 音量アップ
		MUTE,												// ミュート
		REWIND = 0x31,										// 早戻し
		FORWARD,											// 早送り
		MODE_CHANGE = 0x41,									// コントロールモード変更
	};

	//*** コントロールモード列挙型
	enum CONTROL_MODE
	{
		VOLUME,												// 音量コントロール：ダウン/ミュート/アップ
		TRACK,												// トラックコントロール：前/PAUSE/次
		SPEED,												// メディアコントロール：早戻し/通常/早送り
	};

	//*** ESP-NOW Slave/Controllerモード列挙型
	enum ESP_MODE
	{
		NO_INIT = 0,										// 初期化されていない
		SLAVE,												// ESP-NOW Slave
		CONTROLLER,											// ESP-NOW Controller
	};

	//*** 画面モード列挙型
	enum DISP_MODE
	{
		CORE = 0x10,										// Core系
		STICKC = 0x20,										// StickC系
		ATOMS3 = 0x30,										// AtomS3
		COREINK = 0x40,										// CORE.INK
//		DIAL = 0x50,										// DIAL
//		TAB5 = 0x60,										// TAB5
	};

	//*** ボタン識別子列挙型
	enum BUTTON
	{
		BUTTON_NONE = 0,
		BUTTON_A,
		BUTTON_B,
		BUTTON_C,
		BUTTON_EXT,
		BUTTON_PWR,
		BUTTON_MAX,											// BUTTON列挙型要素数
	};

	//*** 座標情報構造体
	struct AREA_INFO
	{
		uint32_t iXstt;										// 開始X座標
		uint32_t iYstt;										// 開始Y座標
		uint32_t iXend;										// 終了X座標
		uint32_t iYend;										// 終了Y座標
		uint32_t iWidth;									// 幅
		uint32_t iHeight;									// 高さ
	};

	//*** エリア2のステータス表示
	enum DRAW_STATUS
	{
		STATUS_USBOTG,										// USB-OTGモード
		STATUS_ESPMODE,										// ESP-NOW Slave/Controllerモード
		STATUS_WIFI,										// WiFi
		STATUS_CTRLMODE,									// コントロールモード
#if defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
		STATUS_HMI_MODULE,									// HMIモジュール
		STATUS_ENCODER_MODULE,								// エンコーダーモジュール
#elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
		STATUS_MINI_ENCODER,								// MiniエンコーダーHAT
		STATUS_MINI_JOYC,									// MiniジョイスティックHAT
#endif	//defined(ARDUINO_M5STACK_CORE) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_CORES3)
		STATUS_ENCODER,										// エンコーダーUNIT
		STATUS_JOYSTICK,									// ジョイスティックUNIT
	};

	//*** I2Cデバイスのオブジェクト。エンコーダーユニット/エンコーダーHATは派生クラスを使用。
	MY_I2C_ENCODER_UNIT* oEncoderUnit = NULL;				// エンコーダーユニット for ALL(派生クラス)
	MY_I2C_ENV_UNIT_S* oEnvUnit_S = NULL;					// ENV.Ⅲユニット(温湿度センサー分) for ALL(派生クラス)
#if false
	MY_I2C_ENV_UNIT_Q* oEnvUnit_Q = NULL;					// ENV.Ⅲユニット(気圧センサー分) for ALL(派生クラス)
#endif
	MY_I2C_HMI_MODULE* oHmiModule = NULL;					// HMIモジュール for Core
	MY_I2C_ENCODER_MODULE* oEncoderModule = NULL;			// エンコーダーモジュール for Core
	MY_I2C_ENCODER_HAT* oEncoderHat = NULL;					// エンコーダーHAT for StickC(派生クラス)
	MY_I2C_Device* oMiniJoycHat = NULL;						// Mini JoycHAT for StickC
//	MY_I2C_Device* oJoystickHat = NULL;						// ジョイスティックHAT for StickC

	//*** ボード情報/ディスプレイ情報
	// ボードの種類/インデックス取得
	m5::board_t boardType = m5gfx::board_unknown;			// 自身のボード種類
	int32_t dispIdx = -1;									// 自身の Displayインデックス

	//*** 外部ディスプレイ情報
#if defined(__M5GFX_M5UNITMINIOLED__)
	int32_t dispIdx1 = -1;									// Displayインデックス
#if !defined(MINIOLED_UNIT_OWN)
//	M5GFX& Display1 = M5.Display;							// M5GFXインスタンス
#else
	M5UnitMiniOLED* oMiniOledUnit = NULL;
#endif	//defined(MINIOLED_UNIT_OWN)
#endif	//defined(__M5GFX_M5UNITMINIOLED__)

	//*** 変数
	// 表示エリア情報
	AREA_INFO ar1;											// 表示エリア1：上
	AREA_INFO ar2;											// 表示エリア2：中
	AREA_INFO ar3;											// 表示エリア3：下
#if defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
	AREA_INFO ar4;											// 表示エリア4：画面下半分
#endif	//defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)

	//*** ESP-NOW関連で使用する変数
	// 自身の情報
	char nickName[32];										// 自身のニックネーム
	uint8_t macAddr[6 + 1];									// 自身のMACアドレス
	ESP_MODE espMode = ESP_MODE::NO_INIT;					// Slave/Controllerモード
	uint32_t requestMacTime = 0;							// Slaveへ MACアドレス要求を送信した時間(millis()をセット)
	int8_t slaveIdx = -1;									// Slaveの Index

	//*** コントロールモード
	CONTROL_MODE controlMode = CONTROL_MODE::VOLUME;		// 音量

	// フラグ
	bool bIn_I2C[128] = { 0x00 };							// In_I2C.scanID()の結果
	bool bEx_I2C[128] = { 0x00 };							// Ex_I2C.scanID()の結果
	bool bWiFi = false;										// WiFiの状態
	bool bEspNow = false;									// ESP-NOWの初期化状態
};
#endif	//!defined(__my_class_h__)
