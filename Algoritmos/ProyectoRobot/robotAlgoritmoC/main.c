#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>     
#include <string.h>   

// --- DEFINICIONES Y CONSTANTES ---
#define N_MAX 10 
#define MAX_TIPOS_OBJETO 10 // Maximo de obstaculos personalizados

// Tipos de Objeto Base (IDs fijos)
#define VACIO 0     
#define MUEBLE -1
#define ESTANTE -2 

// --- ESTRUCTURAS DE DATOS ---

// Registro para almacenar una coordenada (fila, columna)
typedef struct {
    int fila;
    int col;
} Posicion;

// Registro para almacenar los detalles de A* de cada celda
typedef struct {
    int g; // Costo real desde el inicio
    int h; // Costo estimado hasta el final
    int f; // Costo total (f = g + h)
    Posicion padre; // De que celda vino (para reconstruir el camino)
} Celda;

// Registro para definir un tipo de obstaculo
typedef struct {
    int id;                 // ID Negativo (-1, -2, ...)
    char nombre[50];
    char simbolo;           // Caracter para mostrar en el mapa
} ObstaculoEspecial;


// --- VARIABLES GLOBALES DEL SIMULADOR ---
int N; // Dimension de la matriz (ej: 10 para 10x10)
int matriz[N_MAX][N_MAX];       // Mapa principal (guarda 0, -1, -2...)
int costos_pasaje[N_MAX][N_MAX]; // Mapa de costos (guarda 1 o INT_MAX)
Celda detalles_celdas[N_MAX][N_MAX]; // Mapa con detalles para A* (g, h, f)
int cerrado[N_MAX][N_MAX]; // Mapa para marcar celdas ya visitadas por A*

Posicion camino_final[N_MAX * N_MAX]; // Almacen temporal para el camino de un segmento
int pasos_final = 0; // Contador para el camino temporal

Posicion camino_completo[N_MAX * N_MAX * MAX_TIPOS_OBJETO];
int pasos_camino_completo = 0; // Contador para la ruta final

// Lista de todos los obstaculos definidos
ObstaculoEspecial lista_obstaculos[MAX_TIPOS_OBJETO];
int num_tipos_obstaculo = 0; // Cuantos obstaculos hay en la lista

// Arrays para movimiento (Arriba, Abajo, Izquierda, Derecha)
int dir_fila[] = {-1, 1, 0, 0};
int dir_col[] = {0, 0, -1, 1};

// --- Validacion ---

// Verifica si una celda (fila, col) esta dentro de los limites de la matriz
int es_valido(int fila, int col) {
    return (fila >= 0) && (fila < N) && (col >= 0) && (col < N);
}

// Verifica si la celda actual es la meta
int es_destino(int fila, int col, Posicion destino) {
    return (fila == destino.fila) && (col == destino.col);
}

// --- FUNCION HEURISTICA A* ---
// Calcula la "Distancia Manhattan" (costo estimado) desde una celda al destino
int calcular_h(int fila, int col, Posicion destino) {
    return abs(fila - destino.fila) + abs(col - destino.col);
}

// --- FUNCION AUXILIAR A* (Reconstruir camino) ---
// Una vez A* llega a la meta, esta funcion sigue los "padres"
// hacia atras para guardar el camino en el array 'camino_final'
void reconstruir_camino(Posicion inicio, Posicion destino) {
    Posicion p = destino;
    pasos_final = 0;
    while (p.fila != inicio.fila || p.col != inicio.col) {
        camino_final[pasos_final++] = p;
        p = detalles_celdas[p.fila][p.col].padre;
    }
    camino_final[pasos_final++] = inicio;
}

// --- FUNCION DE PRE-PROCESAMIENTO DE COSTOS ---
// Lee la 'matriz' (con IDs) y crea el 'costos_pasaje' (con 1 o INT_MAX)
// Este es el paso crucial que le dice a A* que los obstaculos son imposibles de pasar.
void calcular_costos_colindantes(int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (matriz[i][j] == VACIO) {
                costos_pasaje[i][j] = 1; // Costo base para moverse
            } else { // Cualquier valor negativo es un obstaculo
                costos_pasaje[i][j] = INT_MAX; // Bloqueado (costo infinito)
            }
        }
    }
}

// --- ALGORITMO A* (Logica Principal) ---
int a_star(Posicion inicio, Posicion destino) {
    
    // 1. Validacion inicial
    if (!es_valido(inicio.fila, inicio.col) || costos_pasaje[inicio.fila][inicio.col] == INT_MAX ||
        !es_valido(destino.fila, destino.col) || costos_pasaje[destino.fila][destino.col] == INT_MAX) {
        printf("Error: Inicio o destino (%d,%d) no validos (es un obstaculo o esta fuera de limites).\n", destino.fila, destino.col);
        return 0;
    }

    // 2. Inicializar todas las celdas con costo infinito
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            detalles_celdas[i][j].f = INT_MAX;
            detalles_celdas[i][j].g = INT_MAX;
            detalles_celdas[i][j].padre.fila = -1;
            cerrado[i][j] = 0;
        }
    }

    // 3. Configurar la celda inicial
    int i = inicio.fila;
    int j = inicio.col;
    detalles_celdas[i][j].g = 0; // Costo para llegar al inicio es 0
    detalles_celdas[i][j].h = calcular_h(i, j, destino); // Costo estimado
    detalles_celdas[i][j].f = detalles_celdas[i][j].g + detalles_celdas[i][j].h; // Costo total
    detalles_celdas[i][j].padre = inicio;

    // 4. Crear la Lista Abierta (nodos por visitar) y agregar el inicio
    Posicion lista_abierta[N_MAX * N_MAX];
    int tam_lista_abierta = 0;
    lista_abierta[tam_lista_abierta++] = inicio;
    
    // 5. Bucle principal de A*
    while (tam_lista_abierta > 0) {
        
        // 5.1. Encontrar la celda con el menor costo F en la Lista Abierta
        int min_f = INT_MAX;
        int idx_menor = -1;
        for (int k = 0; k < tam_lista_abierta; k++) {
            Posicion p = lista_abierta[k];
            if (detalles_celdas[p.fila][p.col].f < min_f) {
                min_f = detalles_celdas[p.fila][p.col].f;
                idx_menor = k;
            }
        }

        // 5.2. Sacar la celda elegida ('actual') de la Lista Abierta
        Posicion actual = lista_abierta[idx_menor];
        lista_abierta[idx_menor] = lista_abierta[--tam_lista_abierta];

        // 5.3. Mover 'actual' a la Lista Cerrada (ya la procesamos)
        i = actual.fila;
        j = actual.col;
        cerrado[i][j] = 1;

        // 5.4. Revisar los 4 vecinos
        for (int k = 0; k < 4; k++) {
            int nueva_fila = i + dir_fila[k];
            int nueva_col = j + dir_col[k];

            // Si el vecino es valido...
            if (es_valido(nueva_fila, nueva_col)) {
                // Chequear si es un obstaculo (costo infinito)
                int costo_paso = costos_pasaje[nueva_fila][nueva_col];
                if (costo_paso == INT_MAX) {
                    continue; // Ignorar este vecino (es una pared/obstaculo)
                }

                // Chequear si es el destino
                if (es_destino(nueva_fila, nueva_col, destino)) {
                    detalles_celdas[nueva_fila][nueva_col].padre = actual;
                    detalles_celdas[nueva_fila][nueva_col].g = detalles_celdas[i][j].g + 1; 
                    reconstruir_camino(inicio, destino); // Guardar el camino
                    return 1; // ¡Exito!
                }

                // Si el vecino no esta en la Lista Cerrada
                if (!cerrado[nueva_fila][nueva_col]) {
                    int g_nuevo = detalles_celdas[i][j].g + 1; // Costo nuevo

                    // Si este camino es mejor que el anterior...
                    if (detalles_celdas[nueva_fila][nueva_col].f == INT_MAX || detalles_celdas[nueva_fila][nueva_col].g > g_nuevo) {
                        // ...actualizar sus costos y su padre
                        detalles_celdas[nueva_fila][nueva_col].g = g_nuevo;
                        detalles_celdas[nueva_fila][nueva_col].h = calcular_h(nueva_fila, nueva_col, destino);
                        detalles_celdas[nueva_fila][nueva_col].f = detalles_celdas[nueva_fila][nueva_col].g + detalles_celdas[nueva_fila][nueva_col].h;
                        detalles_celdas[nueva_fila][nueva_col].padre = actual;
                        
                        // Agregar a la Lista Abierta si no estaba
                        int ya_en_lista = 0;
                        for(int l = 0; l < tam_lista_abierta; l++){
                            if(lista_abierta[l].fila == nueva_fila && lista_abierta[l].col == nueva_col){
                                ya_en_lista = 1;
                                break;
                            }
                        }
                        if(!ya_en_lista){
                            lista_abierta[tam_lista_abierta++] = (Posicion){nueva_fila, nueva_col};
                        }
                    }
                }
            }
        }
    }
    return 0; // Fracaso (Lista Abierta vacia, no hay camino)
}

// --- FUNCIONES DE DIBUJO (Terminal) ---

// Busca en la lista de obstaculos y devuelve el simbolo (X, |, #)
char obtener_simbolo_obstaculo(int id_obstaculo) {
     if (id_obstaculo >= 0) return '.'; // No es obstaculo
     int index = -id_obstaculo - 1; // ID -1 es indice 0, ID -2 es indice 1...
     if (index < num_tipos_obstaculo) {
         return lista_obstaculos[index].simbolo;
     }
     return '#'; // Simbolo por defecto si no lo encuentra
}


// Dibuja el mapa ANTES de encontrar el camino
void dibujar_mapa_terminal(int N, Posicion robot_pos, Posicion inicio, Posicion destino) {
    // system("cls"); // Opcional: Limpiar pantalla en Windows
    // system("clear"); // Opcional: Limpiar pantalla en Linux/Mac

    printf("\nMapa Actual:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i == robot_pos.fila && j == robot_pos.col) {
                printf("[-.-]"); // Robot
            } else if (i == inicio.fila && j == inicio.col) {
                 printf("[ I ]"); // Inicio
            } else if (i == destino.fila && j == destino.col) {
                 printf("[ F ]"); // Fin
            } else {
                int tipo_matriz = matriz[i][j];
                if (tipo_matriz == VACIO) {
                    printf("[ . ]"); // Vacio
                } else { // Es un obstaculo
                     char simbolo = obtener_simbolo_obstaculo(tipo_matriz);
                     printf("[ %c ]", simbolo); 
                }
            }
        }
        printf("\n");
    }
     printf("\nLeyenda: [-.-]=Robot, [I]=Inicio, [F]=Meta, [.]=Vacio");
     for(int i = 0; i < num_tipos_obstaculo; i++) {
         printf(", [%c]=%s", lista_obstaculos[i].simbolo, lista_obstaculos[i].nombre);
     }
     printf("\n");
}

// Dibuja el mapa DESPUES de encontrar el camino (con flechas)
void dibujar_camino_final(int N, Posicion inicio, Posicion destino) {
     printf("\n--- Camino Encontrado --- \n");
     char display_map[N_MAX][N_MAX];

     // 1. Inicializar mapa con simbolos base
     for(int i=0; i<N; i++) {
         for(int j=0; j<N; j++) {
             if (matriz[i][j] == VACIO) {
                 display_map[i][j] = '.';
             } else {
                 display_map[i][j] = obtener_simbolo_obstaculo(matriz[i][j]);
             }
         }
     }

     // 2. Dibujar las flechas del camino (usa 'camino_completo')
     for(int k = 0; k < pasos_camino_completo - 1; k++) {
         Posicion actual = camino_completo[k];
         Posicion siguiente = camino_completo[k+1]; 

         // Evitar sobrescribir el inicio
         if(actual.fila == inicio.fila && actual.col == inicio.col) continue;

         if (siguiente.fila > actual.fila) display_map[actual.fila][actual.col] = 'v'; 
         else if (siguiente.fila < actual.fila) display_map[actual.fila][actual.col] = '^'; 
         else if (siguiente.col > actual.col) display_map[actual.fila][actual.col] = '>'; 
         else if (siguiente.col < actual.col) display_map[actual.fila][actual.col] = '<'; 
     }

     // 3. Marcar Inicio y Meta
     display_map[inicio.fila][inicio.col] = 'I';
     display_map[destino.fila][destino.col] = 'F';

     // 4. Imprimir el mapa final con cuadrados
     for(int i=0; i<N; i++) {
         for(int j=0; j<N; j++) {
             printf("[ %c ]", display_map[i][j]); 
         }
         printf("\n");
     }
     printf("\nLeyenda: [I]=Inicio, [F]=Meta, Flechas=Camino, [.]=Vacio");
     for(int i = 0; i < num_tipos_obstaculo; i++) {
         printf(", [%c]=%s", lista_obstaculos[i].simbolo, lista_obstaculos[i].nombre);
     }
     printf("\n");
}


// --- FUNCION DE CONFIGURACION INICIAL ---
// Carga los obstaculos base "mueble" y "estante" en la lista
void inicializar_objetos_default() {
    
    // Obstaculo 1: Mueble
    lista_obstaculos[0].id = 1; 
    strcpy(lista_obstaculos[0].nombre, "mueble");
    lista_obstaculos[0].simbolo = 'X';
    
    // Obstaculo 2: Estante
    lista_obstaculos[1].id = 2; 
    strcpy(lista_obstaculos[1].nombre, "estante");
    lista_obstaculos[1].simbolo = '|'; 
    
    num_tipos_obstaculo = 2; // Tenemos 2 obstaculos por defecto

    printf("Obstaculos por defecto 'mueble' [X] y 'estante' [|] cargados.\n");
}


// --- FUNCION PRINCIPAL (main) ---
int main() {
    #undef main 

    srand(time(NULL)); // Inicializa el generador aleatorio
    inicializar_objetos_default(); // Carga 'mueble' y 'estante'
    
    int opcion_menu;

    // Bucle del Menu Principal
    while(1) {
        
        printf("\n--- SIMULADOR DE ROBOT A* (Terminal) ---\n");
        printf("1: Comenzar simulacion\n");
        printf("2: Definir nuevo Obstaculo (bloqueante)\n"); 
        printf("3: Eliminar obstaculo personalizado\n");    
        printf("4: Salir\n");                              
        printf("Opcion: ");
        
        if (scanf("%d", &opcion_menu) != 1) {
             printf("Entrada no valida. Saliendo.\n");
             while(getchar() != '\n'); 
             break;
        }

        // --- OPCION 1: COMENZAR SIMULACION ---
        if (opcion_menu == 1) {
            
            int r_inicio, c_inicio, r_destino, c_destino;
            int r, c, tipo; 

            printf("\n--- Iniciando Simulacion ---\n");
            
            // Pedir dimension de la matriz
            do {
                printf("Ingrese la dimension de la matriz (N, max %d): ", N_MAX);
                if (scanf("%d", &N) != 1 || N <= 0 || N > N_MAX) {
                    printf("Dimension no valida. Intentelo de nuevo.\n");
                    while (getchar() != '\n'); 
                } else {
                    break;
                }
            } while(1);
            
            // Limpiar la matriz a VACIO
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    matriz[i][j] = VACIO;
                }
            }

            // Pedir Inicio y Fin
            printf("\nPosicion INICIO (fila col): ");
            scanf("%d %d", &r_inicio, &c_inicio);
            printf("Posicion DESTINO (fila col): ");
            scanf("%d %d", &r_destino, &c_destino);

            Posicion inicio = {r_inicio, c_inicio};
            Posicion destino = {r_destino, c_destino};
            
            // Pedir Puntos Intermedios (Waypoints)
            int num_waypoints = 0;
            Posicion waypoints[MAX_TIPOS_OBJETO]; 
            
            printf("\n¿Cuantos puntos intermedios desea agregar? (0 si ninguno): ");
            scanf("%d", &num_waypoints);
            
            if(num_waypoints > MAX_TIPOS_OBJETO) {
                printf("Demasiados puntos, se usara el maximo (%d)\n", MAX_TIPOS_OBJETO);
                num_waypoints = MAX_TIPOS_OBJETO;
            }

            for(int i = 0; i < num_waypoints; i++) {
                printf("Punto intermedio %d (fila col): ", i+1);
                scanf("%d %d", &waypoints[i].fila, &waypoints[i].col);
                if(!es_valido(waypoints[i].fila, waypoints[i].col)) {
                    printf("Posicion no valida. Intentelo de nuevo.\n");
                    i--; 
                }
            }

            // Pedir modo de insercion de obstaculos
            int opcion_objetos; 
            printf("\n--- ¿Como agregar Obstaculos al Mapa? ---\n");
            printf("1: Manualmente\n");
            printf("2: Aleatoriamente (Se generaran 5)\n");
            printf("Opcion: ");
            scanf("%d", &opcion_objetos);

            // 1.1: MODO MANUAL
            if (opcion_objetos == 1) {
                printf("\n--- Codigos de Obstaculos Disponibles ---\n");
                for(int i = 0; i < num_tipos_obstaculo; i++) {
                    printf("%d: %s [%c]\n", -lista_obstaculos[i].id, lista_obstaculos[i].nombre, lista_obstaculos[i].simbolo);
                }
                printf("---------------------------------------\n");
                
                int num_a_ingresar;
                printf("¿Cuantos obstaculos desea ingresar?: ");
                scanf("%d", &num_a_ingresar);
                
                for (int k = 0; k < num_a_ingresar; k++) {
                    printf("Obstaculo %d: Tipo y Posicion (Tipo Fila Col): ", k + 1);
                    
                    if (scanf("%d %d %d", &tipo, &r, &c) != 3 || !es_valido(r, c) || 
                        (tipo >= 0 || tipo < -num_tipos_obstaculo) ) { 
                        
                        printf("Entrada o tipo no valido (debe ser negativo). Saltando.\n");
                        k--;
                        while(getchar() != '\n'); 
                        continue;
                    }
                    if ((r == inicio.fila && c == inicio.col) || (r == destino.fila && c == destino.col)) {
                        printf("Error: No se puede colocar un obstaculo en el inicio o destino.\n");
                        k--;
                        continue;
                    }
                    
                    // Logica especial para 'estante'
                    if (tipo == ESTANTE) { 
                         int tamano, orientacion;
                         printf("ESTANTE seleccionado.\n"); 
                         printf("Ingrese tamano (longitud): ");
                         scanf("%d", &tamano);
                         printf("Orientacion (1=Horizontal, 2=Vertical): ");
                         scanf("%d", &orientacion);

                         int valido = 1;
                         if (orientacion == 1) { 
                             if (c + tamano > N) valido = 0; 
                             for(int col_pared = c; col_pared < c + tamano; col_pared++) {
                                 if (!es_valido(r, col_pared) || matriz[r][col_pared] != VACIO) valido = 0; 
                             }
                         } else if (orientacion == 2) { 
                              if (r + tamano > N) valido = 0; 
                              for(int fila_pared = r; fila_pared < r + tamano; fila_pared++) {
                                  if (!es_valido(fila_pared, c) || matriz[fila_pared][c] != VACIO) valido = 0; 
                              }
                         } else {
                              valido = 0; 
                         }

                         if (!valido) {
                              printf("Posicion/Tamano de estante invalido. Intentelo de nuevo.\n"); 
                              k--; 
                              continue;
                         }

                         if (orientacion == 1) {
                              for(int col_pared = c; col_pared < c + tamano; col_pared++) {
                                  matriz[r][col_pared] = ESTANTE; 
                              }
                         } else {
                              for(int fila_pared = r; fila_pared < r + tamano; fila_pared++) {
                                  matriz[fila_pared][c] = ESTANTE; 
                              }
                         }
                         printf("Estante colocado.\n"); 

                    } else { // Si es Mueble u otro obstaculo
                        matriz[r][c] = tipo;
                    }
                }
            } 
            // 1.2: MODO ALEATORIO
            else {
                int num_a_generar = 5; 
                printf("Generando %d obstaculos aleatorios...\n", num_a_generar);
                
                if(num_tipos_obstaculo == 0) { 
                     printf("No hay tipos de obstaculos definidos para generar.\n");
                } else {
                    for (int k = 0; k < num_a_generar; k++) {
                        do {
                            r = rand() % N; 
                            c = rand() % N; 
                        } while (matriz[r][c] != VACIO || (r == inicio.fila && c == inicio.col) || (r == destino.fila && c == destino.col));
                        
                        int idx_aleatorio = rand() % num_tipos_obstaculo; 
                        int tipo_final = -lista_obstaculos[idx_aleatorio].id; 
                        char* nombre_final = lista_obstaculos[idx_aleatorio].nombre;
                            
                        if (tipo_final == ESTANTE) { // No generar estantes aleatorios
                            k--; 
                            continue;
                        }
                        
                        matriz[r][c] = tipo_final;
                        printf("... Obstaculo %d: %s en (%d, %d)\n", k+1, nombre_final, r, c);
                    }
                }
                printf("--------------------------\n");
            }
            
            // --- FASE DE EJECUCION DE LA SIMULACION ---
            
            // Fase 1: Calcular costos (Marcar obstaculos como INT_MAX)
            calcular_costos_colindantes(N);
            
            // Fase 2: Dibujar estado inicial
            dibujar_mapa_terminal(N, inicio, inicio, destino);

            // Fase 3: Construir la ruta total (I -> P1 -> P2 -> F)
            Posicion ruta_completa[MAX_TIPOS_OBJETO + 2];
            int num_puntos_ruta = 0;
            
            ruta_completa[num_puntos_ruta++] = inicio;
            for(int i = 0; i < num_waypoints; i++) {
                ruta_completa[num_puntos_ruta++] = waypoints[i];
            }
            ruta_completa[num_puntos_ruta++] = destino;
            
            pasos_camino_completo = 0; // Resetea el contador del camino total
            int costo_total_acumulado = 0;
            int todo_ok = 1; // Flag para saber si se pudo completar la ruta

            printf("\n--- Ejecutando Busqueda de Camino por Segmentos ---\n");

            // Fase 4: Calcular A* para cada segmento de la ruta
            for(int i = 0; i < num_puntos_ruta - 1; i++) {
                Posicion punto_a = ruta_completa[i];
                Posicion punto_b = ruta_completa[i+1];
                
                printf("Calculando segmento: (%d,%d) -> (%d,%d)...\n", punto_a.fila, punto_a.col, punto_b.fila, punto_b.col);

                // Llamar a A* para este segmento
                if (!a_star(punto_a, punto_b)) {
                    printf("¡FALLO! No se encontro un camino posible al punto (%d,%d).\n", punto_b.fila, punto_b.col);
                    todo_ok = 0;
                    break; // Salir del bucle for
                }
                
                // Acumular costo total
                costo_total_acumulado += detalles_celdas[punto_b.fila][punto_b.col].g;
                
                // "Pegar" el camino del segmento (que esta al reves) al 'camino_completo'
                for (int j = pasos_final - 1; j >= 0; j--) {
                    if (i > 0 && j == pasos_final - 1) { // Evitar duplicar el punto de union
                        continue;
                    }
                    camino_completo[pasos_camino_completo] = camino_final[j];
                    pasos_camino_completo++;
                }
            }

            // Fase 5: Mostrar resultados
            if (!todo_ok) {
                 printf("Simulacion fallida. Volviendo al menu.\n");
                 continue;
            }
            
            printf("¡Camino completo encontrado! Costo Total (pasos): %d\n", costo_total_acumulado);
            
            printf("\n--- Pasos del Robot ---\n");
             for(int i = 0; i < pasos_camino_completo; i++){
                  printf("Paso a: (%d, %d)\n", camino_completo[i].fila, camino_completo[i].col);
             }
             
             dibujar_camino_final(N, inicio, destino); // Dibuja el mapa con flechas
            
             printf("\nSimulacion completada. Presione Enter para volver al menu...");
             while(getchar() != '\n'); 
             getchar(); 

        // --- OPCION 2: DEFINIR OBSTACULO ---
        } else if (opcion_menu == 2) { 
             if (num_tipos_obstaculo >= MAX_TIPOS_OBJETO) {
                printf("Maximo de tipos de obstaculos (%d) alcanzado.\n", MAX_TIPOS_OBJETO);
                continue; 
            }
            
            ObstaculoEspecial* nuevo_obstaculo = &lista_obstaculos[num_tipos_obstaculo];
            char nombre_buffer[50];
            char simbolo_buffer;

            printf("Nombre del nuevo obstaculo (ej: roca): ");
            scanf("%s", nombre_buffer);
            
            if(strcmp(nombre_buffer, "estante") == 0 || strcmp(nombre_buffer, "mueble") == 0) {
                 printf("Error: 'estante' y 'mueble' son nombres reservados.\n");
                 continue;
            }
            int duplicado = 0;
            for (int i = 0; i < num_tipos_obstaculo; i++) {
                 if (strcmp(nombre_buffer, lista_obstaculos[i].nombre) == 0) {
                    duplicado = 1; break;
                }
            }
            if (duplicado) {
                printf("Error: Un obstaculo con el nombre '%s' ya existe.\n", nombre_buffer);
                continue; 
            }

            printf("Simbolo para representar '%s' en el mapa (un caracter, ej: #): ", nombre_buffer);
            scanf(" %c", &simbolo_buffer); 

            strcpy(nuevo_obstaculo->nombre, nombre_buffer);
            nuevo_obstaculo->id = num_tipos_obstaculo + 1; 
            nuevo_obstaculo->simbolo = simbolo_buffer;
            
            printf("Obstaculo '%s' (ID -%d, Simbolo [%c]) creado.\n", 
                   nuevo_obstaculo->nombre, nuevo_obstaculo->id, nuevo_obstaculo->simbolo);
            
            num_tipos_obstaculo++;

        // --- OPCION 3: ELIMINAR OBSTACULO ---
        } else if (opcion_menu == 3) { 
            if (num_tipos_obstaculo <= 2) { 
                printf("No hay obstaculos personalizados para eliminar.\n");
                continue;
            }
            printf("Obstaculos personalizados disponibles para eliminar:\n");
            for (int i = 2; i < num_tipos_obstaculo; i++) { 
                printf("ID -%d: %s [%c]\n", lista_obstaculos[i].id, lista_obstaculos[i].nombre, lista_obstaculos[i].simbolo);
            }
            printf("Ingrese el ID del obstaculo a eliminar (ej: 3 para ID -3): ");
            int id_eliminar; 
            scanf("%d", &id_eliminar);
            
            if (id_eliminar <= 2 || id_eliminar > num_tipos_obstaculo) {
                printf("ID no valido o es un obstaculo base (mueble, estante).\n");
            } else {
                int idx_eliminar = id_eliminar - 1; 
                printf("Eliminando '%s'...\n", lista_obstaculos[idx_eliminar].nombre);
                for (int i = idx_eliminar; i < num_tipos_obstaculo - 1; i++) {
                    lista_obstaculos[i] = lista_obstaculos[i+1];
                    lista_obstaculos[i].id--; 
                }
                num_tipos_obstaculo--;
                printf("Eliminado con exito.\n");
            }
            
        // --- OPCION 4: SALIR ---
        } else if (opcion_menu == 4) { 
            printf("Saliendo del simulador...\n");
            break; 
            
        } else {
            printf("Opcion no valida. Intentelo de nuevo.\n");
        }
    } 
    
    return 0;
}
