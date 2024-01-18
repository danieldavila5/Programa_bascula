// Primero incluimos las librerías necesarias. La de Grove LCD es para poder sacar resultados por la pantalla LCD

#include "mbed.h"
#include "Grove_LCD_RGB_Backlight.h"
#include <cstdio>

// Una vez se han instalado las librerías, vamos a definir todas las entradas y salidas que vamos a 
// tener en nuestro programa. Serán el LED, el botón de entrada, el servo, la entrada de tensión por el peso
// de la báscula, la pantalla y la conexión con la otra placa.

DigitalOut led(D3);
DigitalIn boton(D4);
DigitalIn boton2(D2);
AnalogIn pot(A0);
PwmOut servomotor(D5); //Salida PWM concentada el el servomotor
Grove_LCD_RGB_Backlight Pantalla(PB_9,PB_8); //El switch de la placa de grove hay que seleccionarlo en 5V para que se ilumine bien
static UnbufferedSerial serial_port(PC_0, PC_1,9600);

// Como nuestro programa va a funcionar como una máquina de estados, definimos los estados:

enum estados{normal, pulsado, espera, calibracion, pulsado2, calibrado0, calibrado100} estado;

#define periodo 1000
#define WAIT_TIME_MS 5 

// Por último antes de comenzar con el main, definimos todas las variables de tipo entero, float o cadena de caracteres
// que vamos a necesitar durante el programa. Las cadenas de caracteres las usaremos para las salidas por la pantalla LCD
// También necesitaremos un timer para el paso del estado normal hasta el de calibrado.

Timer time1;

int Modo; // El modo para lo que scamos por pantalla LCD
float peso; // Peso sin calibrar
float peso_calibrado; // Peso calibrado
int pulsador; // Variable para guardar pulsos y cambiar de estados
int calibrado; // Variable para guardar pulsos y cambiar de estados
float tension_0g=0; // Valor de la tension leida sin peso colocado en la báscula
float tension_100g=1; // Valor de la tension leida con 100g colocados en la báscula
float m; // Valor de la pendiente de la recta
float b; // Valor del término independiente de la recta
float peso_cal=0; // Valor intermedio del peso calibrado hecha la media 
float peso_cali=0; // Valor intermedio del peso calibrado hecha la media
float potm=0;

// Las cadenas de caracteres que saldrán en la pantalla según el estado en el que nos encontremos:

char cadena1[32];
char cadena2[32];
char cadena3[32];
char cadena4[32];
char cadena5[32];
char cadena6[32];

// Declaramos las funciones antes del main aunque las definimos después:

void fnormal();
void fpulsado();
void fespera();
void fcalibrado();
void fcalibrado0();
void fcalibrado100();
void pantalla_control(int Modo);


int main()
{
    // En esta primera parte del main inicializaremos los valores de algunas variables que nos interesa que siempre
    // comiencen con este valor por defecto

     led = 0;
     estado = normal;
     Modo = 1;
     sprintf(cadena1,"Sin peso   ");
     sprintf(cadena2,"Peso 100g");
     sprintf(cadena3,"Peso NC: ");
     sprintf(cadena4,"Peso calibrado: ");

    while (true)
    {

        // Calcuklamos la media de valores obtenidos del peso colocado en la báscula como se hara luego en las funciones
        // de calibrado para obtener datos más exactos. Se usará una recta lineal de tipo y = mx + n

        for (int i=0; i<1000; i++) {
        peso = pot.read()*100;
        m = (tension_100g - tension_0g)*100/(100 - 0);
        b = tension_0g - m*0;
        peso_cal = (pot.read()*1/m - b)*100;
        peso_cali = peso_cal+peso_cali;

        potm=potm+peso;
        }
        
        peso_calibrado=peso_cali/1000; 
        peso_cali=0;
        potm=potm/1000;

        servomotor.period_ms(50);
        servomotor.pulsewidth_us(peso_calibrado*18+500);/////para usar pot *1800
        Pantalla.clear(); 
        
        // Definimos también las cadenas que nos queda, estas deben definirse dentro del while porque su valor cambia en cada 
        // bucle; son los valores del peso colocado.

        sprintf(cadena5,"%.2f ",potm);
        sprintf(cadena6,"%.2f", peso_calibrado);
        pantalla_control(Modo);

        // Utilizaremos un switch para, dependiendo de en que estado estemos, ejecutar la función correspondiente y así ir saltando
        // de estado en estado tal y como nos dice la máquina de estados que define el funcionamiento del programa.

        switch (estado){
            case normal:
            fnormal();
            break;
            case pulsado:
            fpulsado();
            break;
            case espera:
            fespera();
            break;
            case calibracion:
            fcalibrado();
            break;
            case calibrado0:
            fcalibrado0();
            break;
            case calibrado100:
            fcalibrado100();
            break;

        }
        
    }
}

// En la funcion de pantalla control, le decimos a la LCD que dependiendo de si está calibrado o no saque una información
// distinta por pantalla. Le cambiamos el color para que se note el cambio de estado.

void pantalla_control(int Modo){
    if(Modo == 1)
    {
        Pantalla.setRGB(0x00, 0x00, 0xff);  
        Pantalla.locate(0,0);        
        Pantalla.print(cadena3);
        Pantalla.setRGB(0x00, 0x00, 0xff);  
        Pantalla.locate(0,1);        
        Pantalla.print(cadena5);
        thread_sleep_for(100);
    }
    else if (Modo == 2)
    {
        Pantalla.setRGB(0xff, 0x00, 0xff);  
        Pantalla.locate(0,0);        
        Pantalla.print(cadena4);  
        Pantalla.setRGB(0xff, 0x00, 0xff);  
        Pantalla.locate(0,1);        
        Pantalla.print(cadena6); 
        thread_sleep_for(100); 
    }
       
}

// El estado normal es desde donde empezamos, si se pulsa el boton pasamos a pulsado

void fnormal(){
    
    led = 0;
    time1.reset();
    
    if(boton == 1){
        Pantalla.clear();
        estado = pulsado;
    }
}

// Una vez en pulsado empezamos el timer y pasamos a espera

void fpulsado() {
    
    printf("pulsado");        
    time1.start();
    estado = espera;
    
}

// En esta función se puede pasar a calibrado si el boton ha estado pulsado un segundo o volver al modo normal si no es asi

void fespera(){
    printf("esperando");
    
    if(boton == 1 && time1.read() > 1){
        estado = calibracion;
    }
    else if(boton == 0 && time1.read() < 1){
        estado = normal;
    }
}

// El modo calibrado es en el que se comienzan a guardar los datos para obtener los datos reales. Primero, una vez estamos
// en el modo calibrado esperamos a que se deje de pulsar el boton, ya que para entrar en el modo hay que mantenerlo pulsado.
// Una vez dentro del modo se ha soltado, si se vuelve a apretar pasamos al modo calibración 0g. 

void fcalibrado(){
    Modo = 1;
    printf("calibrado\n");
    time1.reset();  
    if(boton == 0 ){
        led = 1;
        calibrado = 1;
        pulsador = 0;
        printf("Estamos en calibracion\n");
    }

    else if(boton == 1 && calibrado == 1){
        estado = calibrado0;
    }
   
}

// Aquí indicaremos al usuario que, sin colocar nignun peso en la bascula guarde el valor de la tensión pulsando de nuevo el boton.
// Al igual que en la anterior función, se debe de haber soltado previamente el boton dentro del estado. 

void fcalibrado0(){

    Pantalla.clear();   
    Modo = 3;
    
    if(boton == 0){
        printf("calibrado a 0_1\n");
        Pantalla.setRGB(0x00, 0x00, 0xff);  
        Pantalla.locate(0,0);        
        Pantalla.print(cadena1);
        thread_sleep_for(100);
        pulsador = 1;
    }
    else if(boton == 1 && pulsador == 1){
               float tension_0ref=0;
               float tension_0refm=0;
        
        // Para obtener valores más excatos y que no varíen tanto haremos la media cada 1000 medidas recogidas:

        for (int i=0; i<1000; i++) {
            tension_0ref = pot.read(); 
            tension_0refm=tension_0refm+tension_0ref;
        }
        

        tension_0g=tension_0refm/1000; 
        tension_0refm=0;
        estado = calibrado100;
    }
        
    }

// Una vez guardado el valor de la tensión para 0 g de peso, haremos lo mismo para el máximo peso. Se indicará que se coloquen 
// 100 g en la báscula y se guardará el dato. Como novedad en este punto, hemos de pulsar el boton de la otra placa unida
// por la comunicación serie para poder salir de este modo, volver al normal y así mostrar los datos de los pesos calibrados
// segun se vayan colocando. 

void fcalibrado100(){

    Pantalla.clear();
  
    printf("calibrado 100\n");
    if(boton == 0){
        Pantalla.setRGB(0x00, 0x00, 0xff);  
        Pantalla.locate(0,0);        
        Pantalla.print(cadena2);
        thread_sleep_for(100);
        pulsador = 2;
    }
    else if(boton == 1 && pulsador == 2){
               float tension_100ref=0;
               float tension_100refm=0;
        
        // Al igual que en el anterior, también haremos una media para minimizar el error:

        for (int i=0; i<1000; i++) {
            tension_100ref = pot.read(); 
            tension_100refm=tension_100refm+tension_100ref;
        }
        

        tension_100g=tension_100refm/1000; 
        tension_100refm=0;

        //estado = normal;
        //Modo = 2;
        Pantalla.clear();
        pulsador = 3;

        
    }

    // En este caso tenemos aqui la condición para que no se vuelva al modo normal hasta que además de guardar el valor de 100 g
    // no se haya pulsado en la otra placa
    else if(serial_port.readable()==true && pulsador == 3){
        estado = normal;
        Modo = 2;
        }
        
        

}