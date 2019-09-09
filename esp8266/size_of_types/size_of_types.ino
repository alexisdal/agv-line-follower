

void setup() {
  Serial.begin(112500);
  Serial.print("\n");
  Serial.print("start\n");

  byte a = 0;
  Serial.print("byte \t");
  Serial.print(sizeof(a));
  Serial.print("\n");

  char b = '\0';
  Serial.print("char \t");
  Serial.print(sizeof(b));
  Serial.print("\n");

  int c = 0;
  Serial.print("int  \t");
  Serial.print(sizeof(c));
  Serial.print("\n");
  
  float d = 0.0f;
  Serial.print("float\t");
  Serial.print(sizeof(d));
  Serial.print("\n");

  double e = 0.0;
  Serial.print("double\t");
  Serial.print(sizeof(e));
  Serial.print("\n");
  
  long f = 0;
  Serial.print("long  \t");
  Serial.print(sizeof(f));
  Serial.print("\n");

  Serial.print("============\n");
  
  size_t g = 0;
  Serial.print("size_t\t");
  Serial.print(sizeof(g));
  Serial.print("\n");
  
  int8_t h = 0;
  Serial.print("int8_t\t");
  Serial.print(sizeof(h));
  Serial.print("\n");

  uint8_t i = 0;
  Serial.print("uint8_t\t");
  Serial.print(sizeof(i));
  Serial.print("\n");
  
  int16_t j = 0;
  Serial.print("int16_t\t");
  Serial.print(sizeof(j));
  Serial.print("\n");
  
  int32_t k = 0;
  Serial.print("int32_t\t");
  Serial.print(sizeof(k));
  Serial.print("\n");
  
  int64_t l = 0;
  Serial.print("int64_t\t");
  Serial.print(sizeof(l));
  Serial.print("\n");
  
}

void loop() {}

/*
// above code produces the following output
// ESP8266
14:25:07.378 -> start
14:25:07.378 -> byte 	1
14:25:07.378 -> char 	1
14:25:07.378 -> int  	4
14:25:07.378 -> float	4
14:25:07.378 -> double	8
14:25:07.378 -> long  	4
14:25:07.378 -> ============
14:25:07.378 -> size_t	4
14:25:07.378 -> int8_t	1
14:25:07.378 -> uint8_t	1
14:25:07.378 -> int16_t	2
14:25:07.378 -> int32_t	4
14:25:07.378 -> int64_t	8

// Arduino Mega
14:29:09.736 -> start
14:29:09.736 -> byte 	1
14:29:09.736 -> char 	1
14:29:09.736 -> int  	2
14:29:09.736 -> float	4
14:29:09.736 -> double	4
14:29:09.736 -> long  	4
14:29:09.736 -> ============
14:29:09.736 -> size_t	2
14:29:09.736 -> int8_t	1
14:29:09.736 -> uint8_t	1
14:29:09.736 -> int16_t	2
14:29:09.736 -> int32_t	4
14:29:09.736 -> int64_t	8

// Arduino Uno
14:55:21.490 -> byte 	1
14:55:21.490 -> char 	1
14:55:21.490 -> int  	2
14:55:21.490 -> float	4
14:55:21.490 -> double	4
14:55:21.490 -> long  	4
14:55:21.490 -> ============
14:55:21.490 -> size_t	2
14:55:21.524 -> int8_t	1
14:55:21.524 -> uint8_t	1
14:55:21.524 -> int16_t	2
14:55:21.524 -> int32_t	4
14:55:21.524 -> int64_t	8
*/