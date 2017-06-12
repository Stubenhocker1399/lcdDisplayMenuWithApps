#define BUTTON_A 14
#define BUTTON_B 10
#define LCD_LED_PIN 21
#define ENCODER_A 4
#define ENCODER_B 17
#define MAX_FOLDER_NAME_LENGTH 50
#define MAX_APPS_PER_CATEGORY 10
#define ENCODER_SUB_STEPS 4
typedef struct{
	volatile int running;
	int fd;
	struct encoder *encoder;
}arg_struct;

enum appType {
	shell, //App not visible for the menu
	basic, //App in the top level of the menu
	menu //App in the lower level of the
};

typedef struct {
	enum appType app_type;
	char *name;
}appInfo;