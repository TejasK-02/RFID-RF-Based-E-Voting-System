#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>

#define RST_PIN 27
#define SS_PIN 5
#define BUZZER 26

// LEDs
int leds[] = {12, 13, 14, 15};
// Buttons
int buttons[] = {33, 32, 25, 4};
String parties[] = {"BJP", "INC", "AAP", "NOTA"};
int votes[4] = {0, 0, 0, 0};

// Master card UID
String masterUID = "33EDA6D";

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

const char* ssid = "TejasS23";
const char* password = "123456789";
String sheetURL = "https://script.google.com/macros/s/AKfycbxp1hC_1_Kb9F1buncDfLuXfGl2yaUmO_cj3gbC80ti2uvO_sBUgnSSMxHilJh-n2VD/exec";

String validCards[] = {"83A51021", "63D4C2DF", "836B79E4", "A3BAD211", "B373DFDF"};
String voterNames[] = {"Tejas Kole", "Harsh Kolhe", "Tejas Kotgire", "Shraddha Kshirsagar", "Testing Card"};
bool hasVoted[] = {false, false, false, false, false};

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wifi Connected");

  SPI.begin();
  mfrc522.PCD_Init();

  for (int i = 0; i < 4; i++) {
    pinMode(leds[i], OUTPUT);
    pinMode(buttons[i], INPUT_PULLUP);
    digitalWrite(leds[i], LOW);
  }

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan your card");
}

void loop() {
  if (digitalRead(0) == LOW) {  // EN button reset
    for (int i = 0; i < 4; i++) votes[i] = 0;
    for (int i = 0; i < 5; i++) hasVoted[i] = false;
    sendSummaryToSheet();
    lcd.clear();
    lcd.print("Sheet Reset Done");
    delay(500);
    lcd.clear();
    lcd.print("Scan your card");
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) uid += String(mfrc522.uid.uidByte[i], HEX);
  uid.toUpperCase();

  Serial.print("Card UID: ");
  Serial.println(uid);
  blinkAll();
  beepOnce();

  if (uid == masterUID) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Admin Access");
  lcd.setCursor(0, 1);
  lcd.print("Sending Summary");
  delay(500);

  sendSummaryToSheet();  // Sheet me data jaa raha hai

  // LCD par winner bhi dikhao
      int totalVotes = votes[0] + votes[1] + votes[2] + votes[3];
    String lcdWinnerText = "";

    if (totalVotes == 0) {
      lcdWinnerText = "No votes cast";
    } else {
      int maxVotes = 0;
      int winnerCount = 0;
      String tiedParties = "";

      for (int i = 0; i < 4; i++) {
        if (votes[i] > maxVotes) {
          maxVotes = votes[i];
        }
      }

      for (int i = 0; i < 4; i++) {
        if (votes[i] == maxVotes) {
          winnerCount++;
          tiedParties += parties[i] + " ";
        }
      }

      if (winnerCount == 1) {
        lcdWinnerText = "Winner: " + tiedParties;
      } else {
        lcdWinnerText = "Tie: " + tiedParties;
      }
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Result Summary:");
    lcd.setCursor(0, 1);
    lcd.print(lcdWinnerText.substring(0, 16)); // Only 16 chars fit on LCD

  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Votes Summary");
  lcd.setCursor(0, 1);
  lcd.print("Sent to Sheet");
  delay(500);

  lcd.clear();
  lcd.print("Scan your card");
  return;
}



  int voterIndex = getVoterIndex(uid);
  if (voterIndex == -1) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Invalid card!");
  Serial.println("Invalid card!");
  doubleBlink();

  sendDataToSheet("Unknown", uid, "-", "❌ Invalid Card");

  delay(500);  // Small delay for display
  lcd.clear();
  lcd.print("Scan your card");
  return;
}


  if (hasVoted[voterIndex]) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Already voted");
  Serial.println("Already voted.");
  doubleBlink();

  sendDataToSheet(voterNames[voterIndex], uid, "-", "❌ Already voted");
  delay(500);
  lcd.clear();
  lcd.print("Scan your card");
  return;
}




  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome:");
  lcd.setCursor(0, 1);
  lcd.print(voterNames[voterIndex]);
  delay(500);
  lcd.clear();
Serial.println("Please vote using buttons (BJP/INC/AAP/NOTA)...");

  int countdown = 10;
  unsigned long lastUpdate = millis();
  bool voted = false;
  unsigned long startTime = millis();

  while (!voted && millis() - startTime < 10000) { // 10 sec timeout
    if (millis() - lastUpdate >= 1000) {
      lastUpdate = millis();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1:BJP  2:INC");
      lcd.setCursor(0, 1);
      lcd.print("3:AAP 4:NOTA");

      lcd.setCursor(13, 0); // show countdown
      lcd.print(countdown);
      countdown--;
    }

    for (int i = 0; i < 4; i++) {
      if (digitalRead(buttons[i]) == LOW) {
        votes[i]++;
        hasVoted[voterIndex] = true;
        digitalWrite(leds[i], HIGH);
        beepOnce();
        delay(500);
        digitalWrite(leds[i], LOW);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Vote Casted:");
        lcd.setCursor(0, 1);
        lcd.print(parties[i]);
        Serial.print("🗳 Vote Casted for Candidate ");
        Serial.println(parties[i]);

        String voteWithEmoji = parties[i];
        if (i == 0) voteWithEmoji = "🪷 BJP";
        else if (i == 1) voteWithEmoji = "✋ INC";
        else if (i == 2) voteWithEmoji = "🧹 AAP";
        else if (i == 3) voteWithEmoji = "🚫 NOTA";

        sendDataToSheet(voterNames[voterIndex], uid, voteWithEmoji, "✅ Casted");

        delay(500);
        lcd.clear();
        lcd.print("Scan your card");
        voted = true;
        break;
      }
    }
  }

  if (!voted) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    lcd.setCursor(0, 1);
    lcd.print("Try again...");
    Serial.println("Vote timeout.");
    tripleBlinkAndBeep();
    delay(1000);
    lcd.clear();
    lcd.print("Scan your card");
  }

}

void blinkAll() {
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 4; i++) digitalWrite(leds[i], HIGH);
    delay(200);
    for (int i = 0; i < 4; i++) digitalWrite(leds[i], LOW);
    delay(200);
  }
}

void beepOnce() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

void doubleBlink() {
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 4; i++) digitalWrite(leds[i], HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(150);
    for (int i = 0; i < 4; i++) digitalWrite(leds[i], LOW);
    digitalWrite(BUZZER, LOW);
    delay(150);
  }
}

void tripleBlinkAndBeep() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) digitalWrite(leds[j], HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(200);
    for (int j = 0; j < 4; j++) digitalWrite(leds[j], LOW);
    digitalWrite(BUZZER, LOW);
    delay(200);
  }
}

int getVoterIndex(String uid) {
  for (int i = 0; i < 5; i++) {
    if (uid == validCards[i]) return i;
  }
  return -1;
}

void sendDataToSheet(String name, String uid, String vote, String status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = sheetURL + "?name=" + urlencode(name) + "&uid=" + uid + "&vote=" + urlencode(vote) + "&status=" + urlencode(status);

   Serial.println("📡 Sending to Google Sheet:");
    Serial.println(url);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.print("Response: ");
      Serial.println(http.getString());
    } else {
      Serial.println("Error sending data.");
    }
    http.end();
  }
}

void sendSummaryToSheet() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Check if no one voted
int totalVotes = votes[0] + votes[1] + votes[2] + votes[3];
String winnerWithEmoji = "";

if (totalVotes == 0) {
  winnerWithEmoji = "❌ No votes cast";
} else {
  int maxVotes = 0;
  int winnerCount = 0;
  String tiedParties = "";

  // Find max vote count
  for (int i = 0; i < 4; i++) {
    if (votes[i] > maxVotes) {
      maxVotes = votes[i];
    }
  }

  // Count how many parties have max votes (for tie)
  for (int i = 0; i < 4; i++) {
    if (votes[i] == maxVotes) {
      winnerCount++;
      if (i == 0) tiedParties += "🪷 BJP, ";
      else if (i == 1) tiedParties += "✋ INC, ";
      else if (i == 2) tiedParties += "🧹 AAP, ";
      else if (i == 3) tiedParties += "🚫 NOTA, ";
    }
  }

  // Remove last comma
  if (tiedParties.endsWith(", ")) tiedParties = tiedParties.substring(0, tiedParties.length() - 2);

  if (winnerCount == 1) {
    winnerWithEmoji = tiedParties;  // Single winner
  } else {
    winnerWithEmoji = "🤝 Tie between: " + tiedParties;
  }
}


    // Create status message
    String status = "🗳️ Final Vote Summary:\n";
status += "🪷 BJP: " + String(votes[0]) + "\n";
status += "✋ INC: " + String(votes[1]) + "\n";
status += "🧹 AAP: " + String(votes[2]) + "\n";
status += "🚫 NOTA: " + String(votes[3]) + "\n\n";
status += "🏆 Winner : " + winnerWithEmoji;





    String url = sheetURL + "?name=Mastercard&uid=33EDA6D&vote=--&status=" + urlencode(status);
    url.replace("\n", "%0A");

    Serial.println("📡 Sending Summary to Google Sheet...");
    Serial.println(url);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.println("Summary sent to sheet.");
    } else {
      Serial.println("Error sending summary.");
    }

    http.end();
  }
}


  String urlencode(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      code0 += code0 > 9 ? 'A' - 10 : '0';
      code1 += code1 > 9 ? 'A' - 10 : '0';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

  


  

