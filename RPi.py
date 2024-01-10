import RPi.GPIO as GPIO
import time
from firebase_admin import credentials, initialize_app, db

# Replace the following with your Firebase project credentials
cred = credentials.Certificate("/home/pi/Desktop/pmain/predictive-maintainence-1841d-firebase-adminsdk-oejc1-37178fae8b.json")
firebase_app = initialize_app(cred, {"databaseURL": "https://predictive-maintainence-1841d-default-rtdb.firebaseio.com/"})

# Replace this with the path where you want to store sensor data in Firebase
sensor_data_ref = db.reference("/")

# Set the GPIO mode to BCM
GPIO.setmode(GPIO.BCM)

# Define the GPIO pin to which the RPM sensor is connected
rpm_sensor_pin = 13

# Set up the GPIO pin as input
GPIO.setup(rpm_sensor_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# Define the GPIO pins to which the buttons are connected
s1 = 17
s2 = 27
s3 = 22
belt = 0
pos = 5
motor = 6

# Set up the GPIO pins as inputs with pull-up resistors
GPIO.setup(s1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(s2, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(s3, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(belt, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(pos, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(motor, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# Initialize variables
rpm_count = 0
prev_time = time.time()

# Define a callback function for the RPM sensor interrupt
def rpm_callback(channel):
    global rpm_count
    rpm_count += 1

# Add event detection to the RPM sensor GPIO pin
GPIO.add_event_detect(rpm_sensor_pin, GPIO.FALLING, callback=rpm_callback)

try:
    while True:
        # Calculate RPM every 5 seconds
        current_time = time.time()
        elapsed_time = current_time - prev_time
        prev_time = current_time

        # Calculate RPM
        rpm = int(((rpm_count / 2) / elapsed_time * 60)/5.7)
        print("RPM:",rpm)

        # Reset RPM count
        rpm_count = 0

        # Check the state of pins s1, s2, and s3 to determine sound value
        if GPIO.input(s1) == 0 and GPIO.input(s2) == 0 and GPIO.input(s3) == 0:
            sound = 0
        elif GPIO.input(s1) == 0 and GPIO.input(s2) == 0 and GPIO.input(s3) == 1:
            sound = 1 
        elif GPIO.input(s1) == 0 and GPIO.input(s2) == 1 and GPIO.input(s3) == 0:
            sound = 2
        elif GPIO.input(s1) == 0 and GPIO.input(s2) == 1 and GPIO.input(s3) == 1:
            sound = 3
        elif GPIO.input(s1) == 1 and GPIO.input(s2) == 0 and GPIO.input(s3) == 0:
            sound = 4 
        elif GPIO.input(s1) == 1 and GPIO.input(s2) == 0 and GPIO.input(s3) == 1:
            sound = 5 
        elif GPIO.input(s1) == 1 and GPIO.input(s2) == 1 and GPIO.input(s3) == 0:
            sound = 6 
        elif GPIO.input(s1) == 1 and GPIO.input(s2) == 1 and GPIO.input(s3) == 1:
            sound = 8
        
        #BELT
        if GPIO.input(belt) == 1:
            Belt = 1
        else:
            Belt = 0
                    
        
        #POSITION
        if GPIO.input(pos) == 1:
            position = 1
        else:
            position = 0


        #POSITION
        if GPIO.input(motor) == 1:
            temp = 1
        else:
            temp = 0
            

        sensor_data_ref.child("SOUND DECIBEL").set(sound)
        sensor_data_ref.child("BELT POSITION").set(Belt)
        sensor_data_ref.child("RPM").set(rpm)
        sensor_data_ref.child("POSITION").set(position)
        sensor_data_ref.child("MOTOR TEMPERATURE").set(temp)
        time.sleep(1)  # Upload data every 60 seconds (adjust as needed)

except KeyboardInterrupt:
    print("Program terminated by user.")
finally:
    firebase_app.delete()  # Clean up Firebase resources
    GPIO.cleanup()
