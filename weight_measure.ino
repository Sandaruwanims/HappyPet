void drop_50(){
  //update the weight global variable
  measure();
  
  while(weight < STEP_WEIGHT){
    //drop foods
    for (pos = closed_position; pos >= open_position; pos -= 1) { 
      myservo.write(pos);              
      delay(10);                      
    }
    delay(50);
    //close the lid
    for (pos = open_position; pos <= closed_position; pos += 1) { 
      myservo.write(pos);              
      delay(10);                      
    }
    
    measure();
    delay(50);
  }
}


void measure(){
  
  weight = -scale.get_units();
  //Serial.println(weight, 4);
  
  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == 't' || temp == 'T')
      scale.tare();  //Reset the scale to zero      
  }
}
