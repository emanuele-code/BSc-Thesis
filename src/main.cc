#include <stdint.h>

#include "thread.hpp"
#include "hal.hpp"


static uint8_t dir_sx = 1;
static uint8_t dir_dx = 7;  // in aritmetica modulare l'ultimo indice fa iterare nel verso opposto il buffer circolare
static uint8_t step   = 0;

static Thread<64> t_btn, t_motore, t_led;      // thread btn e motore permanenti, solo led riutilizzato

static CondVar          avvia;
static CondVar          sblocca;


static volatile bool    bloccato    = false;
static volatile bool    motore_gira = false;   // vera dalla richiesta alla fine del quarto di giro
static volatile uint8_t direzione   = 0;       // direzione richiesta al motore

enum LedAttivo : uint8_t { VERDE, ROSSO };
static volatile LedAttivo led_attivo;   // quale blinker è stato lanciato su t_led


/* sequenza a mezzo passo: 4096 passi per un giro completo */
/* lo stepper è attratto dalle bobine attive, leggere l'array al contrario ne inverte il verso */
static const uint8_t seq[8] = { 0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001 };


// accende o spegne le bobine
static void accendi_bobina(uint8_t v) {
    (v & 0b0001) ? DP10::high() : DP10::low();
    (v & 0b0010) ? DP11::high() : DP11::low();
    (v & 0b0100) ? DP12::high() : DP12::low();
    (v & 0b1000) ? DP13::high() : DP13::low();
}




void *motore(void *) {
    for(;;) {
        while(!motore_gira) {      // se il motore non gira
            thread_wait(&avvia);   // aspetta una richiesta dai btn di rotazione
        }

        uint8_t dir = direzione;                // valorizzata da 'premuto'
        for(uint16_t i = 0; i < 1024; i++) {    // ruota 90°
            while(bloccato) {                   // aspetta un operazione
                accendi_bobina(0);              // ferma motore
                thread_wait(&sblocca);          // aspetta lo sblocco dal btn blu
            }
            accendi_bobina(seq[step]);          // riprendi da dove fermato
            thread_sleep(1);                    // cedi per un altro thread
            step = (step + dir) % 8;            // accende la bobina per far avanzare lo stepper
        }
        accendi_bobina(0);                      // ferma la bobina
        motore_gira = false;                    // il blinker rosso finisce, il verde lo rilancia btn
    }
    return nullptr;
}





template <typename PIN>
void *blinker(void *) {
    while(motore_gira && !bloccato) {   // rossi: lampeggiano finché il motore non è bloccato
        PIN::toggle();
        thread_sleep(100);       // cedi cpu
    }
    PIN::low();                  // spegni led
    return nullptr;
}


void *verde(void *) {
    DP8::high();
    while(!motore_gira || bloccato) {   // verde: acceso finché il motore è fermo o bloccato
        thread_sleep(1);                // cedi cpu
    }
    DP8::low();                         // spegni led
    return nullptr;
}





// pulsante di blocco (BLU)
static void controlla_blocco() {
    if(DP3::read()) return;          // btn di blocco non cliccato, ritorna

    bloccato = !bloccato;            // toggle stato
    if(bloccato) {                   // accende led blu (bloccato)
        DP4::high();
    } else {
        DP4::low();                  // spegne led blu e segnala le operazioni
        thread_signal(&sblocca);
    }
    while(!DP3::read()) thread_sleep(20);   // aspetta il rilascio del btn
}





static void premuto(uint8_t *dir) {
    if(bloccato) return;                                       // se bloccato altre azioni non sono consentite
    if(motore_gira || led_attivo != VERDE) return;             // il motore gira, o il blinker rosso non è ancora finito

    direzione   = *dir;                                        // valorizzo la var globale direzione
    motore_gira = true;                                        // richiesta al motore 
    thread_signal(&avvia);                                     // sveglia il thread del motore
}





/* tiene sempre un blinker sullo slot t_led: appena si libera lancia quello giusto per lo stato attuale */
static void aggiorna_led() {
    if(t_led.tcb.state != TERMINATED) return;                  // il blinker precedente non è ancora finito

    if(!motore_gira || bloccato) {                             // motore fermo oppure bloccato: verde
        led_attivo = VERDE;
        thread_detach(thread_create(&t_led, verde, nullptr));  // motore fermo: verde fisso
    } else if(direzione == dir_sx) {
        led_attivo = ROSSO;
        thread_detach(thread_create(&t_led, blinker<DP7>, nullptr));    // giro a sinistra: blinker sinistro
    } else {
        led_attivo = ROSSO;
        thread_detach(thread_create(&t_led, blinker<DP9>, nullptr));    // giro a destra: blinker destro
    }
}




void *btn(void *) {
    for(;;) {
        controlla_blocco();
        aggiorna_led();
        bool sx = !DP6::read();  // controlla se un bottone dx/sx è cliccato
        bool dx = !DP5::read();
        if(sx != dx) premuto(sx ? &dir_sx : &dir_dx); // solo uno cliccato
        thread_sleep(5);         // da spazio agli altri thread per girare
    }
    return nullptr;
}





int main() {
    hal_init();

    DP10::output(); DP11::output(); DP12::output(); DP13::output();  // pin per attivare le bobine
    DP7::output();  DP8::output();  DP9::output();                   // led rosso sx, led verde, led rosso dx
    DP6::input();   DP6::high();                                     // btn sx
    DP5::input();   DP5::high();                                     // btn dx
    DP4::output();                                                   // led blu
    DP3::input();   DP3::high();                                     // btn blocco

    thread_create(&t_btn,    btn,    nullptr);
    thread_create(&t_motore, motore, nullptr);

    thread_start();
}