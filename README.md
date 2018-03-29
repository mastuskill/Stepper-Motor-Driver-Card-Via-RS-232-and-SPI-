A Stepper Motor Driver card needs to be designed and constructed. The card should be controlled via analogue input through SPI and RS-232 serial communication protocols. The controller card generates  pulses to control up to two separate stepper motors in any direction and angle. Simple commands allow the independent motion of the motors. The following commands are transmitted from a workstation terminal. Each respective stepper motor, angle position should be displayed on LCD. The driver card should include digital inputs for connecting limit switches so to limit stepper motor movement. Stepper motor movement should halt when either limit switch is activated. Two LEDs (L0, L1) and buzzer are used to indicate if either stepper motor is moving or not. L0 or L1 should blink with a frequency of 0.2Hz if either limit switch is activated. Additionally buzzer should be activated for 2 seconds. Stepper motor driver card operation should be disabled when a switch SW1 is pressed. System remains disabled until SW1 is pressed again for at least 5 seconds. All commands should be ignored while driver card is disabled.  

Then using Keil IDE and interrupts or RTX multitasking techniques  an embedded C/C++ program was done to implement the required stepper motor driver card functionality  

Basic Commands that the 2 motors can do:
- Rotate clockwise or anticlockwise ( continuous or step by step)
- Speed controlled via ADC using SPI protocol
- Multitasking By using RTX kernel operating system
- Emergency stop
- LED indicators

