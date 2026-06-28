#include <Arduino.h>
// Define Pin Connections
const int trigPin = 5;
const int echoPin = 10;

// Variables for duration and distance
long duration;
int distanceCm;

void setup() {
  Serial.begin(9600); // Start serial communication at 9600 baud
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  // 1. Clear the trigPin by setting it LOW for 2 microseconds
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // 2. Send a 10-microsecond pulse to trigger the sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // 3. Read the echoPin, returning the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // 4. Calculate the distance (Speed of sound is ~0.034 cm/us, divide by 2 for round trip)
  distanceCm = duration * 0.034 / 2;

  // 5. Print the distance to the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");

  // Wait 500ms before taking the next reading
  delay(500);
}

