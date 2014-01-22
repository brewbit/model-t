

#define BOARD_RST    10
#define BOARD_DETECT 7
#define BOARD_BOOT   A1

#define DEBOUNCE_TIME 250
#define DETECT_TIME 500


void setup()
{
  pinMode(BOARD_RST, INPUT);
  pinMode(BOARD_DETECT, INPUT);
  pinMode(BOARD_BOOT, OUTPUT);
  
  digitalWrite(BOARD_BOOT, LOW);
  digitalWrite(BOARD_RST, LOW);
  
  Serial.begin(9600);
}

void loop()
{
  switch (Serial.read()) {
    // wait for connect
    case 'c':
      board_detect(true);
      break;
    
    // wait for disconnect
    case 'd':
      board_detect(false);
      break;
      
    // enter BOOT mode
    case 'b':
      board_power(true);
      break;
      
    // enter RUN mode
    case 'r':
      board_power(false);
      break;
      
    default:
      break;
  }
}

void board_power(boolean boot)
{
  pinMode(BOARD_BOOT, boot ? INPUT : OUTPUT);
  pinMode(BOARD_RST, OUTPUT);
  delay(100);
  pinMode(BOARD_RST, INPUT);
  
  board_detect(true);
}

boolean board_detect(boolean connect)
{
  int desired_state = connect ? HIGH : LOW;
  unsigned long detect_start;
  unsigned long detect_time;
  boolean detected = false;
  
  detect_start = detect_time = millis();
  
  while ((millis() - detect_start) < DETECT_TIME &&
         (millis() - detect_time) < DEBOUNCE_TIME) {
    if (digitalRead(BOARD_DETECT) == desired_state) {
      detect_time = millis();
      detected = true;
    }
    else {
      detected = false;
    }
  }
  
  if (detected)
    Serial.print("1");
  else
    Serial.print("0");
    
  return detected;
}
