//Program to run a brushless motor in closed loop mode as a servomotor with proportional control
//By Juan Pablo Angulo
//Use a Power Stage, such as AO4606 to drive the 3 coils of your motor.
//Can be run on a board such as https://www.ebay.com/itm/124316900944
//Arduino forum thread: https://forum.arduino.cc/index.php?topic=672887.0

const int K=5; // Proportional gain for position control
const int potPin = A1;  // pot controls the setpoint
const int encoder = A0; // AMS AS5048 encoder 
const int motorPin1 =9; //Coil A
const int motorPin2 =10; //Coil B
const int motorPin3 =11; //Coil C
const int P = 11; //Motor's number of Poles 

// Variables
int pwmSin[] = {127,110,94,78,64,50,37,26,17,10,4,1,0,
                1,4,10,17,26,37,50,64,78,94,110,127,
                144,160,176,191,204,217,228,237,244,250,253,254,255,
                250,244,237,228,217,204,191,176,160,144}; // Lookup table array of 48 PWM duty values for 8-bit timer - sine function
int currentStepA=0;  //index for lookup table Coil A 
int currentStepB=16; //index for lookup table Coil B
int currentStepC=32; //index for lookup table Coil C

int pos=0; //mechanical position of shaft
int epos=0; //electrical position of shaft
int torque=0; //output torque
int directn=0; //vector for Magnetic field orientation
int setpoint=0; //variable for storing desired position
int error=0; //variable for calculating the error for Proportional Control
int flag=0;
int T=0;
int posabs=0;
int posold=0;
int diff=0;

void setup() {
  Serial.begin(115200);
  TCCR1B = TCCR1B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 9 and 10
  TCCR2B = TCCR2B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 11 and 3 (3 not used)
  ICR1 = 255 ; // 8 bit resolution
 
  pinMode(potPin, INPUT);
  pinMode(encoder, INPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
}
 
void loop() {

  setpoint = analogRead(potPin); //read potentiometer
  setpoint = map(setpoint,0,1023,360,0); //scale to 0-360 degrees
  setpoint=180; //if no potentiometer, you can define a value for setpoint here.
  posold=pos;  //write the previous position of the shaft into the variable posold to later compare to actual position
  pos=pulseIn(encoder,HIGH);//read encoder pulse
  epos=map(pos,4,900,0,(48*P)); //scale shaft's encoder position range to electrical positions multiple of 48*Number of poles in motor.
  epos=constrain(epos, 0, 48*P); //constraint the values

  //Calculate absolute position
  //Serial.println(T); //debug number of complete turns
  pos=map(epos,0,(48*P),0,360); //translate the electrical position to shaft position in degrees (0-360)
  diff=posold-pos; //calculate the change between old position and new position.
  diff=abs(diff); 
  if (diff>50 && posold>pos && flag==1) T++; //if the change was larger than 50, then consider it as a turn in positive direction
  if (diff>50 && posold<pos && flag==1) T--; //if the change was larger than 50, then consider it as a turn in negative direction
  flag=1; //flag for first iteration's jump in values not to trigger the turn counter.
  posabs=(T*360)+pos; //here we calculate the absolute position taking the current position plus the Turn counter
   
 // Proportional control
  error=posabs-setpoint; //calculate error
  if (error<0)directn=36; //orient magnetic field vector 90 degrees ahead
  if (error>0)directn=12; //orient magnetic field vector 90 degrees behind 
  torque=abs(error*K); //define the magnitude of the reaction of the motor, or the Proportional Gain. Small torque for small errors, large torque for large errors. Adjust the Value of K to better match the response you want from the motor, try 1,2,5,8,10.
  if ((abs(error))<2) torque=0; //Introduce a small deadband to reduce vibrations at the cost of some precision
  torque=constrain(torque,0,50);    //limit maximum torque 0-100 %, you can limit the maximum torque you want the motor to have i.e. instead of 100, use 60 for 60% of the maximum torque available, motor runs slower and cooler ;) 
  move();
}
  
void move()
{
  const int offset=9; //**ADJUST THIS VALUE** OFFSET needed for Syncronization, unique to each motor as it depends on manufacturing.
  currentStepA = epos + directn + offset;  //directn is used to define the rotation the motor sould turn.
  currentStepB = currentStepA + 16; //add 120 deg of phase to StepA position. 
  currentStepC = currentStepA + 32; //add 240 deg of phase to StepA position.
 
  currentStepA = currentStepA%48; //I used remainder operation or modulo to "wrap" the values between 0 and 47
  currentStepB = currentStepB%48;
  currentStepC = currentStepC%48;

 //write PWM values to ports
   
  analogWrite(motorPin1, pwmSin[currentStepA]*torque/100);
  analogWrite(motorPin2, pwmSin[currentStepB]*torque/100);
  analogWrite(motorPin3, pwmSin[currentStepC]*torque/100);
 
 /*
  //for debbuging
  Serial.print("DATA,");
  Serial.print(pwmSin[currentStepA]);
  Serial.print(","); 
  Serial.print(pwmSin[currentStepB]);
  Serial.print(","); 
  Serial.print(pwmSin[currentStepC]);
  Serial.print(","); 
  Serial.println(pos);
  */  
}
