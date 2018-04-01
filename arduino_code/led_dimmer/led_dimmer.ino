#include <Wire.h>

#define LED 5
#define USR1 2
#define USR2 3
#define ADDR1 6
#define ADDR2 7
#define ADDR3 8
#define ADDR4 9
#define ADDR5 10
#define ADDR6 11
#define ADDR7 12

#define DIM_BUTTON USR1
#define OPTION_BUTTON USR2

/* dim button constants */
#define DIM_STEP 1
#define DEBOUNCE_TIME 15
#define TOGGLE_TIME 500
#define DIM_TIME 20
//#define DIM_VALUE_TIME 50
#define DIM_SPEED_FAST 50
#define DIM_SPEED_SLOW 18000
#define MINIMUM_TOGGLE_BRIGHTNESS 10

/* button states*/
#define STATE_INACTIVE 0
#define STATE_DEBOUNCING 1
#define STATE_TOGGLE 2
#define STATE_DIM 3

#define ACTION_NOTHING 0
#define ACTION_TOGGLE 4
#define ACTION_DIM 5
#define ACTION_OPTION 6

#define DIRECTION_UP 1
#define DIRECTION_DOWN 2

/* I2C stuff */
#define I2C_ADDR 42
#define I2C_GENERAL_CALL 0
#define I2C_IDDLE 0
#define I2C_ALL_OFF 1

char i2c_last_received = I2C_IDDLE;


/* board IDs
0: all off
1: all off + wake up slave
2: all on
3: all on + wake up slave
4: wake up master
*/

//board id jumper settings
int jumpers[]={ADDR1, ADDR2, ADDR3, ADDR4, ADDR5, ADDR6, ADDR7};
int board_id = 0;

for (int x : jumpers){
    int jumper_value = !digitalRead(x);
    board_id +=1
     if (jumper_value == HIGH){ 
        break;
     }
     
    }

//dimmer speed setting
int dimmer_speed= DIM_SPEED_SLOW;

/* dimmer target setting */
int dimmer_target = 0;
int dimmer_stored_target = 0;
int dim_direction = DIRECTION_UP;

/* dimmer value setting */
int dimmer_value = 0;
long dimmer_value_millis = 0;

/* reading out the dimmer button */
int dim_button_state = STATE_INACTIVE;
long dim_button_millis = 0;


/* reading out the option button */
int option_button_state = STATE_INACTIVE;
long option_button_millis = 0;

/* Called repetedly when dim button is pressed,
 * sets the target brightness value for the led */
int dimmer_step_target(){
    
    dimmer_speed = DIM_SPEED_SLOW;

	if (dim_direction == DIRECTION_UP){
//		dimmer_target += DIM_STEP;
        dimmer_target = dimmer_value + DIM_STEP; //ensure to avoid jumps in led brightness if dimming started during wake-up cycle
	}
	else if (dim_direction == DIRECTION_DOWN){
		dimmer_target -= DIM_STEP;
	}

	if (dimmer_target <= 0){
		dimmer_target = 0;
		dim_direction = DIRECTION_UP;
	}

	if (dimmer_target >= 255){
		dimmer_target = 255;
		dim_direction = DIRECTION_DOWN;
	}

	dimmer_stored_target = dimmer_target;
    

	Serial.print("Dimmer target: ");
	Serial.println(dimmer_target);
}

/* Called once per loop itteration
 * steps the PWM output towards the target
 * value for the led */
int dimmer_step_value(){
	long now_millis = millis();

//	if (now_millis - dimmer_value_millis > DIM_VALUE_TIME &&
    if (now_millis - dimmer_value_millis > dimmer_speed &&
			dimmer_target != dimmer_value) {
		dimmer_value_millis == now_millis;
		if (dimmer_target > dimmer_value) dimmer_value++;
		if (dimmer_target < dimmer_value) dimmer_value--;

		analogWrite(LED, dimmer_value);

		Serial.print("Dimmer value: ");
		Serial.println(dimmer_value);
	}
}

/* Toggle the led */
int toggle_led() {
    
    dimmer_speed = DIM_SPEED_SLOW;
    
	if (dimmer_target > 0) {
		dimmer_target = 0;
	} else {
		dimmer_target = max(dimmer_stored_target, MINIMUM_TOGGLE_BRIGHTNESS);
	}
}

/* Turn led off, and store dim status */
int all_led_off() {
     
    dimmer_speed = DIM_SPEED_FAST;
    dimmer_target = 0;
    
}

int all_led_on() {
     
    dimmer_speed = DIM_SPEED_FAST;
    dimmer_target = 255;
    
}

// set target and speed for wake up light
int wake_set_target() {
    
    dimmer_speed = DIM_SPEED_SLOW;
    dimmer_target = 255;
    
   
    
}


int option_button_task(){
    switch (board_id){
            case(0 OR 1):
            all_led_off();
            Serial.println("LOCAL ALL LED OFF");
            i2c_broadcast(I2C_ALL_OFF);
            
            case(2 OR 3):
            all_led_on();
            Serial.println("LOCAL ALL LED ON");
            i2c_broadcast(I2C_ALL_ON);
            
            case(4):
            wake_set_target();
            Serial.println("LOCAL WAKE SET TARGET");
            i2c_broadcast(I2C_WAKE_SET);
    }
    
}


/* Read and debounce the dim button (light switch)
 * decide which action (Nothing, Dim or Toggle) to
 * take */
int read_dim_button() {
  int button_value = !digitalRead(DIM_BUTTON);
  long millis_now = millis();
  
  if (button_value == LOW) {
    if (dim_button_state == STATE_TOGGLE) {
      dim_button_state = STATE_INACTIVE;
      return ACTION_TOGGLE;
    }
    dim_button_state = STATE_INACTIVE;
    return ACTION_NOTHING;
  }
  
  if (button_value == HIGH) {
    switch(dim_button_state){
      case STATE_INACTIVE:
        dim_button_millis = millis_now;
        dim_button_state = STATE_DEBOUNCING;
        break;
        
      case STATE_DEBOUNCING:
        if (millis_now - dim_button_millis > DEBOUNCE_TIME) {
          dim_button_millis = millis_now;
          dim_button_state = STATE_TOGGLE;
        }
        break;
        
      case STATE_TOGGLE:
        if (millis_now - dim_button_millis > TOGGLE_TIME) {
          dim_button_millis = millis_now;
          dim_button_state = STATE_DIM;
          return ACTION_DIM;
        }
        break;
        
      case STATE_DIM:
          if (millis_now - dim_button_millis > DIM_TIME) {
          dim_button_millis = millis_now;
          dim_button_state = STATE_DIM;
          return ACTION_DIM;
        }
        break;
    }
  }
  return ACTION_NOTHING;
}


int read_option_button() {
  int button_value = !digitalRead(OPTION_BUTTON);
  long millis_now = millis();

  if (button_value == LOW) {
    if (option_button_state == STATE_TOGGLE) {
      option_button_state = STATE_INACTIVE;
      return ACTION_OPTION;
    }
    option_button_state = STATE_INACTIVE;
    return ACTION_NOTHING;
  }

  if (button_value == HIGH) {
    switch(option_button_state){
      case STATE_INACTIVE:
        option_button_millis = millis_now;
        option_button_state = STATE_DEBOUNCING;
        break;

      case STATE_DEBOUNCING:
        if (millis_now - option_button_millis > DEBOUNCE_TIME) {
          option_button_millis = millis_now;
          option_button_state = STATE_TOGGLE;
        }
        break;
    }
  }
  return ACTION_NOTHING;
}

/* I2C Rx handler */
void i2c_rx_handler(int byteCount){
	/* Note that only last received byte will be acted upon */
	digitalWrite(13, !digitalRead(13));
	while (Wire.available()){
		i2c_last_received =  Wire.read();
	}
}

/* nice wrapper to send i2c commands */
void i2c_broadcast(char data){
	Wire.beginTransmission(I2C_ADDR);
	Wire.write(data);
	Wire.endTransmission();
}


// the setup routine runs once when you press reset:
void setup() {                

  pinMode(LED, OUTPUT);

  Wire.begin(I2C_ADDR);
  Wire.onReceive(i2c_rx_handler);

  Serial.begin(115200);
  Serial.println("Good morning. I am the dimmer");
}

// the loop routine runs over and over again forever:
void loop() {
    int dim_button_action;
	int option_button_action;

	/* DIM BUTTON */
    dim_button_action = read_dim_button();
    if (dim_button_action == ACTION_TOGGLE) {
		Serial.println("TOGGLE");
		toggle_led();
	}
    if (dim_button_action == ACTION_DIM) {
		Serial.println("DIM");
		dimmer_step_target();
	}

	/* OPTION BUTTON */
	option_button_action = read_option_button();
	if (option_button_action == ACTION_OPTION){
		Serial.println("OPTION BUTTON TASK");
		option_button_task();
	}

	/* I2C RECEIVE */
    switch (i2c_last_received){
            case(I2C_ALL_OFF):
            Serial.println("I2C ALL OFF");
            all_led_off();
            
            case(I2C_ALL_ON):
            Serial.println("I2C ALL ON");
            all_led_on();
            
            case(I2C_WAKE_SET):
            Serial.println("I2C WAKE SET");
            wake_set_target();
    
    }
    
/*	if (i2c_last_received == I2C_ALL_OFF) {
		Serial.println("I2C ALL OFF");
		all_led_off();
	}
*/
	i2c_last_received = I2C_IDDLE;

	dimmer_step_value();
    
}