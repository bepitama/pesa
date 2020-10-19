//Created by Giuseppe Tamanini 19/10/2020
//Licenza CC BY-SA 4.0

#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 5;
const int LOADCELL_SCK_PIN = 6;
const int buzzer = 7; //buzzer to arduino pin 4
HX711 scale;

#include "RTClib.h"
#include "HT16K33.h"

RTC_DS1307 rtc;

HT16K33 seg(0x70);

int mis; // misura del peso
int oldmis; // vecchia misura del peso
boolean misinfun; // si sta usando la pesa
int yy; // anno
int mmm; // mese
int dd; // giorno
int hh; // ora
int mm; // min
int mn = 0; // min count down
int ssin;  // sec di Time all'inizio del count down
int sscd; // secondi visualizzati nel count down
int sscu; // secondi visualizzati nel count up
int ss; // secondi
int oldss; // vecchio valore dei secondi
int GMese[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // numero di giorni dei mesi
int gm; // giorni totali mese
int campo; // seleziona il campo di modifica della data/ora
long timeColon; // tempo per il lampeggio dei :
boolean VisPesa = false; // visualizza pesa
boolean VisDate = false; // visualizza ore/data
boolean VisCD = false; // visualizza count down
boolean VisCU = false; // visualizza count up
boolean CU; // è attivo il count up
boolean CD; // è attivo il count down
boolean countdownset; // è in corso l'impostazione dei minuti del count down
const int button1Pin = 2; // pin a cui sono collegati i touch
const int button2Pin = 3;
const int button3Pin = 4;
boolean button1State; // memorizza lo stato dei touch
boolean button2State;
boolean button3State;
boolean button23State;
boolean oldbutton1State; // memorizza lo stato prededente dei touch
boolean oldbutton2State;
boolean oldbutton3State;
boolean oldbutton23State;
long timepressedInButton1; // tempo in cui viene premuto uno o due touch
long timepressedInButton2;
long timepressedInButton3;
long timepressedInButt23;
long timepressedButton1; // tempo di premuta dei touch
long timepressedButton2;
long timepressedButton3;
long timepressedButt23; // tempo di premuta contemporanea dei touch 2 e 3
boolean buttonPressed; // C'è un un touch premuto?
boolean cambiato1; // stato dei touch nella modifica della data/ora
boolean cambiato2;
boolean cambiato3;
long timeNotPressed; // tempo che calcola per quanto non viene premuto nessun pulsante
boolean upupaOff; // se viene premuto il touch 3 del timer interrompe il suono upupa

void setup() {

  Serial.begin(115200);
  
  pinMode(button1Pin, INPUT); // inizializza i pin di input
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);
  
  seg.begin(); // inizializza il display
  Wire.setClock(100000);
  seg.displayOn();
  seg.displayClear();
  seg.brightness(15);
  
  if (! rtc.begin()) { // verifica la presenza dell'RTC
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  // inizializza il modulo HX711 per la lettura del peso
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(395.f);    // imposta la scala
  scale.tare();              // resetta la scala
  scale.tare();
  timeColon = millis(); // tempo di accensione dei due punti (:)
  //attachInterrupt(0, spegneupupa, RISING);
}

void loop() {
  
  DateTime now = rtc.now(); // legge i dati dall'RTC
  yy = now.year();
  mmm = now.month();
  dd = now.day();
  hh = now.hour();
  mm = now.minute();
  ss = now.second();
  
  button1State = digitalRead(button1Pin);
  button2State = digitalRead(button2Pin);
  button3State = digitalRead(button3Pin);
  if (button1State) {
    buttonPressed = true;
  } else if (button2State) {
    buttonPressed = true;
  } else if (button3State) {
    buttonPressed = true;
  } else if (buttonPressed == true) { // se non è premuto nessun touch e in precedenza è stato premuto un touch
    Serial.println("nessuno");
    timeNotPressed = millis(); // calcola il tempo di non premuta
    buttonPressed = false; // non è premuto nessun touch
  }

  if (button2State && button3State) {
    button23State = true;
  } else {
    button23State = false;
  }
  if (button23State && oldbutton23State == false) {
    oldbutton23State = true;
    timepressedInButt23 = millis();
    Serial.println(timepressedInButt23);
  }
  if (button23State == false && oldbutton23State) {
    timepressedButt23 = millis() - timepressedInButt23;
    Serial.println(timepressedButt23);
    oldbutton23State = false;
  }
  if (timepressedButt23 > 3000) {
    timepressedButt23 = 0;
    Serial.println("Modifica data/ora");
    campo = 1;
    timeNotPressed=(millis());
    modificaDataOra();
  }
  if (button1State && oldbutton1State == false) { // se è premuto il touch1 avvia il conto del tempo di premuta
    oldbutton1State = true;
    timepressedInButton1 = millis();
  }
  if (button1State == LOW && oldbutton1State) { // quando il touch1 viene rilasciato calcola il tempo di premuta
    timepressedButton1 = millis() - timepressedInButton1;
    oldbutton1State = false;
  }  
  if (timepressedButton1 > 250 && VisPesa) { // se il touch1 è stato premuto per più di 250 ms e sta funzionando la pesa vie eseguita la tara
    timepressedButton1 = 0;
    scale.tare(); // esegue la tara    
  }
  if (timepressedButton1 > 250) { // se il touch1 è stato premuto per più di 250 ms accende la pesa
    timepressedButton1 = 0;
    scale.power_up(); // accende l'HX711
    VisPesa = true;
    VisDate = false;
    VisCD = false;
  }
  
  if (button2State && oldbutton2State == false) {  // se è premuto il touch2 avvia il conto del tempo di premuta
    oldbutton2State = true;
    timepressedInButton2 = millis();
  }
  if (button2State == LOW && oldbutton2State) {  // quando il touch2 viene rilasciato calcola il tempo di premuta
    timepressedButton2 = millis() - timepressedInButton2;
    oldbutton2State = false;
  }  
  if (timepressedButton2 > 250 && VisDate == false) { // se il touch2 è stato premuto per più di 250 ms visualizza l'ora/data
    timepressedButton2 = 0;
    VisPesa = false;
    VisDate = true;
  }
  
  if (button3State && oldbutton3State == false) {  // se è premuto il touch3 avvia il conto del tempo di premuta
    oldbutton3State = true;
    timepressedInButton3 = millis();
  }
  if (button3State == LOW && oldbutton3State) {  // quando il touch3 viene rilasciato calcola il tempo di premuta
    timepressedButton3 = millis() - timepressedInButton3;
    oldbutton3State = false;
  }
  if (timepressedButton3 > 250 && VisCD == false && CD == false && VisCU == false && CU ==false) { // se il touch2 è stato premuto per più di 5 ms avvia l'impostazione dei minuti del count down
    timepressedButton3 = 0;
    mn = 0;
    countdownset = true; // la modalità set è attiva
    seg.suppressLeadingZeroPlaces(0);
    seg.displayTime(mn, 0);
    seg.displayColon(true);
    VisPesa = false;
    VisDate = false;
    CU = true;
  }
  if (timepressedButton3 > 25 && countdownset && VisCD == false) { // se il touch2 è stato premuto per più di 25 ms e è attiva countdownset aumenta i minuti del count down
    timepressedButton3 = 0;
    VisCU = false;
    CU = false;
    CD = true;
    mn = mn + 1;
    seg.displayTime(mn, 0);
    seg.displayColon(true);
  }

  if (millis() - timeNotPressed > 4000 && buttonPressed == false && VisDate) { // se non è stato premuto nessun touch per 4 sec spegne l'ora
    if (VisCD == false && VisCU == false) {
      seg.displayClear();
    }
    VisDate = false;
  }
  if (millis() - timeNotPressed > 20000 && buttonPressed == false && VisPesa && misinfun == false) { // se non è stato premuto nessun touch per 20 sec e non ci sono variazioni di misura spegne la pesa
    seg.displayClear();
    VisPesa = false;
  }
  if (millis() - timeNotPressed > 3000 && buttonPressed == false && VisCD == false && VisCU == false && CU && CD == false) { // se se si sono impostati i minuti e non è stato premuto alcun touch per 3 sec avvia il count up
    VisPesa = false;
    VisDate = false;
    VisCU = true;
    seg.displayTime(mn,0);
    ssin = ss;
  }  
  if (millis() - timeNotPressed > 3000 && buttonPressed == false && VisCD == false && CU == false && CD) { // se sono stati impostati i minuti e non è stato premuto nessun touch per più di 3 sec avvia il coun down
    VisPesa = false;
    VisDate = false;
    VisCD = true;
    countdownset = false;
    seg.displayTime(mn,0);
    ssin = ss;
  }
  if (timepressedButton3 > 3000 && (VisCD || VisCU)) { // se il touch3 è stato premuto per più di 3 sec ed è in corso il count down o il count up spegne il count
    timepressedButton3 = 0;
    mn = 0;
    VisCD = false; // spegne il count down
    VisCU = false;
    CD = false;
    CU = false;
    seg.displayClear(); // spegne il display
  }
  if (VisPesa) dispPesa(); // avvia la procedura della pesa
  if (VisDate) dispTime(); // avvia la procedura per la visualizzazione dell'ora/data
  if (VisCD) dispCD();
  if (VisCU) dispCU();
}

void dispTime() { //Orologio
  
  seg.suppressLeadingZeroPlaces(0); // visualizza gli zeri non significativi
  
  if (millis() - timeColon > 500) { //accende o spegne i due punti
    seg.displayColon(false);
  } else {
    seg.displayColon(true);
  }  
  if (ss != oldss) { //se è passato un secondo
    seg.displayTime(hh, mm); // visualizza l'ora
    timeColon = millis();
    oldss = ss;
  }

}

void dispCD() {  //CountDown

  seg.suppressLeadingZeroPlaces(0); // visualizza gli zeri non significativi

  if (VisCD) {
    if (millis() - timeColon > 500) { //accende o spegne i due punti
      seg.displayColon(false);
    } else {
      seg.displayColon(true);
    }
    if (ss != oldss) { //se è passato un secondo
      sscd = 60 - ss + ssin;
      if (sscd > 59) {
        sscd = sscd - 60;
      }
      seg.displayTime(mn, sscd);
      timeColon = millis();
      oldss = ss;
      if (sscd == 0) {
        mn = mn - 1;
        Serial.println(mn);
      }
    }
    if (mn == -1 && sscd == 0) {
      seg.displayColon(true);
      mn = 0;
      VisCD = false;
      CD = false;
      upupaOff = false;
      upupa();
      seg.displayClear();
      do { 
        button3State = digitalRead(button3Pin);
      }
      while (button3State);
      button3State = false;
    }
  }

}

void dispCU() {

  seg.suppressLeadingZeroPlaces(0); // visualizza gli zeri non significativi

  if (VisCU) {
    if (millis() - timeColon > 500) { //accende o spegne i due punti
      seg.displayColon(false);
    } else {
      seg.displayColon(true);
    }
    if (ss != oldss) { //se è passato un secondo
      sscu = ss - ssin;
      if (sscu < 0) {
        sscu = sscu + 60;
      }
      Serial.println(ssin);
      seg.displayTime(mn, sscu);
      timeColon = millis();
      oldss = ss;
      if (sscu ==59) {
        mn = mn + 1;
      }
    }
    if (mn == 100 && sscu == 0) {
      seg.displayColon(true);
      mn = 0;
      VisCU = false;
      CU = false;
      seg.displayClear();
    }
  }
  
}

void dispPesa() {

  for (int i = 1; i < 4; i++) {
    seg.suppressLeadingZeroPlaces(i); // non visualizza gli zeri non significativi
  }
  seg.displayColon(false);
  mis = scale.get_units(10), 1;
  seg.displayInt(mis);
  if (abs(mis - oldmis) > 10) {
    oldmis = mis;
    misinfun = true;
    timeNotPressed = millis();
  } else {
    misinfun = false;
  }

}

void upupa() {
  for (int i = 0; i < 6; i++) {
  button3State = digitalRead(button3Pin);
  if (button3State) {
    upupaOff = true;
    return;
  }
  tone(buzzer, 2500); // Suono 2,5KHz
  delay(75);          // per 75 ms
  noTone(buzzer);     // ferma il suono
  button3State = digitalRead(button3Pin);
  if (button3State) {
    upupaOff = true;
    return;
  }
  delay(75);          // silenzio per 75 ms
  button3State = digitalRead(button3Pin);
  if (button3State) {
    upupaOff = true;
    return;
  }
  tone(buzzer, 2500); // ripete altre 2 volte
  delay(75);
  noTone(buzzer);
  button3State = digitalRead(button3Pin);
  if (button3State) {
    upupaOff = true;
    return;
  }
  delay(75);
  button3State = digitalRead(button3Pin);
  if (button3State) {
    upupaOff = true;
    return;
  }
  tone(buzzer, 2500);
  delay(75);
  noTone(buzzer);
  button3State = digitalRead(button3Pin);
  if (button3State) {
    upupaOff = true;
    return;
  }
  delay(500);         // silenzio per mezzo sec
  }
}

void modificaDataOra() {
  while (campo < 7) {
    Serial.println(campo);
    while (campo < 7) {
      switch (campo) {
      case 1:  
        seg.displayInt(yy);
        break;
      case 2:  
        seg.displayInt(mmm);
        break;
      case 3:  
        seg.displayInt(dd);
        break;
      case 4:  
        seg.displayInt(hh);
        break;
      case 5:  
        seg.displayInt(mm);
        break;
      case 6:  
        seg.displayInt(ss);
        break;
      }
      button1State = digitalRead(button1Pin);
      if (button1State && oldbutton1State == false) {
        timeNotPressed = millis(); // tempo di non premuta che provaca l'uscita senza salvare
        oldbutton1State = true; 
      } else if (button1State == false && oldbutton1State) {
        oldbutton1State = false;  // pone il vecchio stato del pulsante1 a falso
        cambiato1 = false; // lo stato dell'azione del pulsante1 è falsa 
      }
      if (button1State && oldbutton1State && cambiato1 == false && millis() - timepressedInButton1 > 10) {
        switch (campo) {
        case 1:
          if (yy > 2019) yy = yy - 1;
          break;
        case 2:
          mmm = mmm - 1;
          if (mmm < 1) mmm = 12;
          break;
        case 3:
          gm = GMese[mmm - 1]; // estrae dalla matrice il numero dei giorni del mese corrente
          if (yy / 4 == int(yy / 4) && mm == 2) gm = gm + 1; // se l'anno è bisestile ed è febbraio aggiungi 1 al numero dei giorni del mese
          dd = dd - 1; // decrementa il numero dei giorni
          if (dd < 1) { // se il valore è minore di 1
            dd = gm; // pone numDay uguale al numero dei giorni del mese corrente
          }
          break;
        case 4:
          hh = hh - 1;
          if (hh < 1) hh = 23;
          break;
        case 5:
          mm = mm - 1;
          if (mm < 1) mm = 59;
          break;
        case 6:
          ss = ss - 1;
          if (ss < 1) ss = 59;
          break;
        }
        cambiato1 = true; // lo stato dell'azione del pulsante1 è vera
      }
      button2State = digitalRead(button2Pin);
      if (button2State && !oldbutton2State) {
        timeNotPressed = millis(); // tempo di non premuta che provaca l'uscita senza salvare
        oldbutton2State = true; 
      } else if (button2State == false && oldbutton2State) {
        oldbutton2State = false;  // pone il vecchio stato del pulsante2 a falso
        cambiato2 = false; // lo stato dell'azione del pulsante2 è falsa 
      }
      if (button2State && oldbutton2State && cambiato2 == false && millis() - timepressedInButton2 > 10) {
        switch (campo) {
        case 1:
          yy = yy + 1;
          break;
        case 2:
          Serial.println(mmm);
          mmm = mmm + 1;
          if (mmm > 12) mmm = 1;
          break;
        case 3:
          dd = dd + 1;
          gm = GMese[mmm - 1];
          if (yy / 4 == int(yy / 4) && mmm == 2) gm = gm + 1; // se l'anno è bisestile ed è febbraio aggiungi 1 al numero dei giorni del mese
          if (dd > gm) dd = 1;
          break;
        case 4:
          hh = hh + 1;
          if (hh > 23) hh = 0;
          break;
        case 5:
          mm = mm + 1;
          if (mm > 59) mm = 0;
          break;
        case 6:
          ss = ss + 1;
          if (ss > 59) ss = 0;
          break;
        }
        cambiato2 = true; // lo stato dell'azione del pulsante2 è vera
      }
      button3State = digitalRead(button3Pin);
      if (button3State && !oldbutton3State) {
        timeNotPressed = millis(); // tempo di non premuta che provoca l'uscita senza salvare
        timepressedInButton3 = millis();
        oldbutton3State = true; 
      } else if (button3State == false && oldbutton3State) {
        oldbutton3State = false;  // pone il vecchio stato del pulsante3 a falso
        cambiato3 = false; // lo stato dell'azione del pulsante3 è falsa 
      }
      if (button3State && oldbutton3State && cambiato3 == false && millis() - timepressedInButton3 > 10) {
        campo = campo + 1;
        cambiato3 = true; // lo stato dell'azione del pulsante3 è vera
      }
      if (millis() - timeNotPressed > 5000) { // se non è stato premuto nessun tasto per 5 sec esce senza salvare
        Serial.print("timeNotPressed");
        Serial.println(millis() - timeNotPressed);
        campo = 7;
      }
    }
  }
  if (cambiato3 && campo == 7) rtc.adjust(DateTime(yy, mmm, dd, hh, mm, ss));
  seg.displayClear();
}
