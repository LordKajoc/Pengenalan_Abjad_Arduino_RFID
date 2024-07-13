#include <SPI.h>
#include <MFRC522.h>
#include "mp3tf16p.h"
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>

//Deklarasi PIN MFRC522
#define SS_PIN 10
#define RST_PIN 9

// Membuat Objek MFRC522 
MFRC522 mfrc522(SS_PIN, RST_PIN); 

// Deklarasi Pin DFPlayer
#define DFPLAYER_RX A2
#define DFPLAYER_TX A3
MP3Player myMP3(DFPLAYER_RX, DFPLAYER_TX);

//deklarasi Pin LCD TFT
#define TFT_DC    7
#define TFT_RST   8 
#define SCR_WD   240
#define SCR_HT   240   // 320 - to allow access to full 240x320 frame buffer
Arduino_ST7789 lcd = Arduino_ST7789(TFT_DC, TFT_RST);

//state ubah mode
int mode = 1;  // Mode 1: 1-26, Mode 2: 27-52
const int buttonPinMode = 4;  // Define pin button mode
int buttonState = 0;

//Button Evaluasi
const int buttonPinEvaluasi = 2;  // Define pin button evaluasi

//Daftar Kode Kartu
const int NUM_CODES = 26;  
byte authorizedRFIDs[NUM_CODES][4] = {
  {0x23, 0xB0, 0xC6, 0x1D},
  {0x23, 0x7B, 0xFA, 0x34},
  {0x64, 0x60, 0xEF, 0xFE},
  {0x04, 0x1B, 0xE7, 0xAF},
  {0x70, 0x3B, 0xCA, 0x3B},
  {0x90, 0x70, 0x32, 0x3B},
  {0x64, 0xB8, 0xC1, 0xCE},
  {0x90, 0x81, 0x95, 0x3B},
  {0x65, 0x15, 0x70, 0xFC},
  {0x84, 0xD8, 0xB6, 0xFE},
  {0x84, 0x47, 0x32, 0xA9},
  {0x80, 0x16, 0xB6, 0x3B},
  {0x7B, 0x98, 0xD5, 0x19},
  {0x7D, 0xB8, 0xB9, 0x3C},
  {0x80, 0x14, 0x06, 0x3C},
  {0x6B, 0x30, 0x96, 0x19},
  {0x84, 0xAF, 0xB2, 0xA9},
  {0x84, 0x4D, 0x24, 0xCF},
  {0x84, 0xE2, 0x1B, 0xFF},
  {0x7D, 0x89, 0x60, 0x6B},
  {0x04, 0x87, 0x1E, 0xB0},
  {0x64, 0x2B, 0xC3, 0xCE},
  {0x74, 0xC9, 0x5A, 0xCE},
  {0x73, 0x93, 0xF4, 0x1D},
  {0x0D, 0x24, 0xF8, 0x6B},
  {0x63, 0xA9, 0x6B, 0x1D}
};

// Untuk mode evaluasi dan indeks data kode kartu
const char* wordsMode1[] = {
  "ANGGUR", "BABI", "CABAI", "DOMBA", "ENOKI", "FLAMINGGO", "GANDUM", "HARIMAU",
  "INTAN", "JERAPAH", "KENTANG", "LANDAK", "MANGGA", "NYAMUK", "ONIGIRI", "PANDA",
  "QUINOA", "RUSA", "SELADA", "TAPIR", "UBI", "VIRUS", "WORTEL", "XYLOPHONE",
  "YOGURT", "ZEBRA", "ANJING", "BAYAM", "CACING", "DURIAN", "ELANG", "FOTO", "GAJAH", 
  "HAZEL", "IGUANA", "JERUK", "KANGGURU", "LEMON", "MONYET", "NANGKA", "OWA", "PAPAYA",
  "QUOKKA", "RASBERI", "SERIGALA", "TEMPE", "UNTA", "VAS", "WOMBAT", "X-RAY",
  "YOYO", "ZIG-ZAG"
};

//variabel global menampung data sementara
const char* chosenWord; //kata yang dipilih ketika evaluasi
char correctAnswer; // huruf/abjad evaluasi
const char* correctAnswerFull = ""; // jawaban dengan kata full
char firstCharacterWord; //huruf pertama dari kata
char wordQuestion[20]; //panjang huruf dari kata pertanyaan

//variable status
bool gameInProgress = false; //status mode game
bool detectInProgress = false; //status mode deteksi


//setup awal alat
void setup() {
  // put your setup code here, to run once:
  pinMode(buttonPinMode, INPUT);
  pinMode(buttonPinEvaluasi, INPUT);
  Serial.begin(9600); // Initialize serial communications with the PC
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  myMP3.initialize(); // Init DFPlayer Mini
  setupLCD();
  myMP3.playTrackNumber(55, 30); // Play audio selamat datang
  Serial.println("Ayo Tempelkan Kartu");
  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:
    //button untuk ubah mode
  buttonState = digitalRead(buttonPinMode);
  if (buttonState == HIGH) {
    mode = (mode == 1) ? 2 : 1;
    delay(500); //delay penekanan buttton agar tidak berulang
    Serial.println(mode);
  }

  //button untuk mode evaluasi
  if (digitalRead(buttonPinEvaluasi) == HIGH) {
    delay(500); //delay penekanan buttton agar tidak berulang
    startGame();
  }

    // mendeteksi kartu baru
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Pilih salah satu kartu
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // cek UID tag
  Serial.print("UID tag :");
  byte readRFID[4];
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    readRFID[i] = mfrc522.uid.uidByte[i];
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  Serial.println(content);

  // cek kartu rfid terdaftar pada list
  bool authorized = false;
  int trackNumber = 0;
  for (int i = 0; i < NUM_CODES; i++) {
    if (memcmp(readRFID, authorizedRFIDs[i], 4) == 0) {
      authorized = true;
      trackNumber = i + 1;
      break;
    }
  }
  if (authorized) {
    Serial.println("Akses Diterima");
    detectInProgress = true;
    if (gameInProgress == true) {
      Serial.println("tracknya nomor: " + trackNumber);
      checkAnswer(trackNumber);
      gameInProgress = false; // Game selesai
    } else {
        if (mode == 2) {
        trackNumber += 26;
      }
      const char* word = getWordForTrackNumber(trackNumber);
      if (word != nullptr) {
        correctAnswerFull = word;
        firstCharacterWord = word[0];
        Serial.println(" - Kata: " + String(word));
        Serial.println("answer: " + String(correctAnswerFull));
        Serial.println("Karakter: " + firstCharacterWord);
      }
      updateLCDWord();
      myMP3.playTrackNumber(trackNumber, 30);
      Serial.println("tracknya nomor: " + trackNumber);
      Serial.print("disini");
      delay(5000);
      detectInProgress = false;
      backToSetup();
      // Menampilkan kata yang sesuai dengan trackNumber
    }
  } else {
    Serial.println("Akses Ditolak");
  }
  delay(1000);
}


//update LCD ketika mode deteksi kartu dan jawaban
void updateLCDWord(){
  lcd.init();
  lcd.fillScreen(WHITE);
  lcd.setRotation(135);
  int textWidth, textHeight;
  getTextBounds(correctAnswerFull, 3, &textWidth, &textHeight);

  int x = (SCR_WD - textWidth) / 2;  // Hitung posisi x untuk menempatkan teks di tengah
  int y = (3 * SCR_HT / 4) - textHeight / 2;  // Hitung posisi y untuk menempatkan teks di tengah

  lcd.setCursor(80, 50);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(15);
  lcd.println(String(firstCharacterWord));

  lcd.setCursor(x, y);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(3);
  lcd.println(correctAnswerFull);
}

// Fungsi untuk mendapatkan kata berdasarkan trackNumber
const char* getWordForTrackNumber(int trackNumber) {
  if (trackNumber >= 1 && trackNumber <= 52) {
    return wordsMode1[trackNumber - 1];
  } else {
    return nullptr;
  }
}

//Fungsi Setup LCD awal
void setupLCD(){
  lcd.init(SCR_WD, SCR_HT);
  lcd.fillScreen(WHITE);
  lcd.setRotation(135);
  lcd.setCursor(90, 70);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(3);
  lcd.println("AYO");

  lcd.setCursor(40, 100);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(3);
  lcd.println("TEMPELKAN");

  lcd.setCursor(80, 130);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(3);
  lcd.println("KARTU");
}

//Fungsi Mode Evaluasi
void startGame() {
  // Memilih random kata
  myMP3.playTrackNumber(56, 30,true); // Play audio mulai permainan
  delay(3000);
  Serial.println("Selesai mulai audio permainan");
  if (mode == 1) {
    chosenWord = wordsMode1[random(0, 26)]; //pemilihan kata random indeks 0-26
  } else {
    chosenWord = wordsMode1[random(27, 52)]; //pemilihan kata random indeks 27-52
  }

  wordQuestion[0] = '_';
  strcpy(&wordQuestion[1], &chosenWord[1]);
  Serial.print("Nilai wordquestion:");
  Serial.println(wordQuestion);
  // Menampilkan Kata dengan hilangnya huruf depan
  Serial.print("Guess the missing letter: ");
  Serial.print("_");
  for (int i = 1; i < strlen(chosenWord); i++) {
    Serial.print(chosenWord[i]);
  }
  Serial.println();
  updateLCDWordEvaluasi();
  delay(1000);
  myMP3.playTrackNumber(57, 30,true); // Play audio mulai permainan
  Serial.println("Selesai mulai audio huruf hilang");
  delay(1000);
  // Menyimpan jawaban benar di global variabel
  correctAnswer = chosenWord[0];
  gameInProgress = true; // Game dimulai
}

//Fungsi cek jawaban atau validasi Mode Evaluasi
void checkAnswer(int trackNumber) {
  char answerLetter;
  int playTrackNumberNow = correctAnswer - 'A' + 1; 
  if (mode == 1) {
    answerLetter = 'A' + (trackNumber - 1);
  } else {
    answerLetter = 'A' + (trackNumber - 1);
  }

  if (answerLetter == correctAnswer) {
    Serial.println("Jawaban Benar!");
    Serial.println(answerLetter);
    Serial.println(correctAnswer);
    Serial.println(playTrackNumberNow);
    if (mode == 1){
      updateLCDAnswer();
      myMP3.playTrackNumber(playTrackNumberNow, 30,true);
      delay(4000);
      Serial.println("disini mode 1");
      myMP3.playTrackNumber(54, 30, true); //audio benar
    }
    else {
    updateLCDAnswer();
    myMP3.playTrackNumber(playTrackNumberNow += 26, 30,true);
    delay(4000);
    Serial.println("disini mode 2");
    myMP3.playTrackNumber(54, 30); //audio benar
    }
  } else {
    Serial.println("Jawabannya Salah");
    Serial.println(answerLetter);
    Serial.println(correctAnswer);
    Serial.println(playTrackNumberNow);
    myMP3.playTrackNumber(53, 30); // Memutar track yang menyatakan jawaban salah
    delay(4000); // Tambahkan delay agar suara memiliki waktu untuk diputar
    if (mode == 1) {
      updateLCDAnswer();
      myMP3.playTrackNumber(playTrackNumberNow, 30); // Memutar track jawaban yang benar mode 1
      Serial.println("disini3");
    } else {
      updateLCDAnswer();
      myMP3.playTrackNumber(playTrackNumberNow += 26, 30); // Memutar track jawaban yang benar mode 2
    }
  }
}


// Fungsi untuk menghitung lebar dan tinggi teks dan tata letak text
void getTextBounds(const char *str, int textSize, int *width, int *height) {
  *width = strlen(str) * 6 * textSize;  // 6 adalah lebar karakter default pada ukuran teks 1
  *height = 8 * textSize;  // 8 adalah tinggi karakter default pada ukuran teks 1
}


//update LCD ketika mode evaluasi dimulai
void updateLCDWordEvaluasi(){
  lcd.init();
  lcd.fillScreen(WHITE);
  lcd.setRotation(135);
  int textWidth, textHeight;
  getTextBounds(wordQuestion, 3, &textWidth, &textHeight);

  int x = (SCR_WD - textWidth) / 2;  // Hitung posisi x untuk menempatkan teks di tengah
  int y = (SCR_HT - textHeight) / 2;  // Hitung posisi y untuk menempatkan teks di tengah

  lcd.setCursor(x, y);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(3);
  lcd.println(wordQuestion); //LCD menampilkan kata pertanyaan
}


//update LCD ketika menjawab evaluasi
void updateLCDAnswer(){
  lcd.init();
  lcd.fillScreen(WHITE);
  lcd.setRotation(135);
  int textWidth, textHeight;
  getTextBounds(chosenWord, 3, &textWidth, &textHeight);

  int x = (SCR_WD - textWidth) / 2;  // Hitung posisi x untuk menempatkan teks di tengah
  int y = (3 * SCR_HT / 4) - textHeight / 2;  // Hitung posisi y untuk menempatkan teks di tengah
  lcd.setCursor(80, 50);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(15);
  lcd.println(String(correctAnswer));

  lcd.setCursor(x, y);
  lcd.setTextColor(BLACK);
  lcd.setTextSize(3);
  lcd.println(chosenWord);
}

//fungsi kembali ke setelan layar awal
void backToSetup() {
  if (gameInProgress == false && detectInProgress == false){
    delay(2000);
    setupLCD();
      Serial.println("Reset to initial setup mode");
  }
}