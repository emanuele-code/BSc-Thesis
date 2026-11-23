#include <stdint.h>

#include "thread.hpp"
#include "hal.hpp"


static CondVar puo_produrre;               
static CondVar puo_consumare;             

static volatile uint8_t disponibili = 0;   




template <typename R1, typename R2, typename R3>
struct Slots {
    static void high(uint8_t i) {
        if(i == 0)      R1::high();  // accendi led Risorsa 1 
        else if(i == 1) R2::high(); 
        else            R3::high();
    }

    static void low(uint8_t i) {
        if(i == 0)      R1::low();  // spegni led Risorsa 1 
        else if(i == 1) R2::low();  
        else            R3::low();
    }
};



template <typename PIN_PROD, typename S>
void* produci(void*) {
    uint8_t idx_prod = 0;   

    for(;;) {
        while(disponibili >= 3)           
            thread_wait(&puo_produrre);

        PIN_PROD::high();               // accende led blu produttore             
        S::high(idx_prod);              // accende i-esimo led verde risorsa         
        thread_sleep(700);                     
        PIN_PROD::low();                // spegne led blu  

        idx_prod = (idx_prod + 1) % 3;  // cambia risorsa
        disponibili++;                      
        thread_signal(&puo_consumare);  // segnala disponibilità 
        thread_yield();                 // cede cpu
    }
    return nullptr;
}



template <typename PIN_CONS, typename S>
void* consuma(void*) {
    uint8_t idx_cons = 0;

    for(;;) {
        thread_sleep(800);

        while(disponibili == 0) 
            thread_wait(&puo_consumare);

        uint8_t n = (disponibili == 3) ? 3 : 1;   // se 3 ris. disp. consuma tutto

        PIN_CONS::high();                         // accende led rosso consumatore
        for(uint8_t i = 0; i < n; i++) {          // itera quante risorse consumare
            S::low(idx_cons);                     
            idx_cons = (idx_cons + 1) % 3;        
            thread_sleep(100);
        }
        thread_sleep(700);
        PIN_CONS::low();                          // spegni led rosso

        disponibili -= n;                         // decrementa risorse
        thread_signal(&puo_produrre);             // segnala consumo
        thread_yield();                           // cede cpu
    }
    return nullptr;
}





static Thread<64> produttore, consumatore;   // riserva memoria

int main() {
    hal_init();        // hardware abstraction layer, per attivare pin, tick e timer

    DP13::output();    // accendi pin a 5V
    DP12::output();
    DP11::output();
    DP10::output();
    DP9::output();

    using Slot = Slots<DP12, DP11, DP10>; 

    thread_create(&produttore,  produci<DP13, Slot>, nullptr);  // valorizza thread
    thread_create(&consumatore, consuma<DP9,  Slot>, nullptr);


    thread_start();    // avvia lo scheduler con una prima thread_yield
    return 0;
}