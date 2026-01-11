import csv

# ==============================================================================
# CONFIGURACIÓN Y CONSTANTES (REQUISITOS DE CÁTEDRA)
# ==============================================================================

# CORRECCION: Requisito PDF  - Mantener grado de multiprogramación en 5.
GRADO_MULTIPROGRAMACION = 5  

# CORRECCION
# Se asume que el SO ocupa los primeros 100K (0-99), por eso la primera partición arranca en 100.
PARTICIONES_REQUISITOS = [
    {"id": "Grande", "tam": 250, "inicio": 100}, 
    {"id": "Mediano", "tam": 150, "inicio": 350},
    {"id": "Pequeño", "tam": 50, "inicio": 500}
]

class Proceso:
    def __init__(self, pid, arribo, irrupcion, memoria):
        self.pid = pid
        self.arribo = int(arribo)
        self.irrupcion = int(irrupcion)
        self.memoria_req = int(memoria)
        self.tiempo_restante = self.irrupcion
        # CORRECCION: Estados manejados: Nuevo, Listo, Ejecución, Terminado, Suspendido.
        self.estado = "Nuevo" 
        self.particion_id = None
        # Variables para estadísticas
        self.tiempo_fin = 0
        self.tiempo_espera = 0
        self.tiempo_retorno = 0

class Particion:
    def __init__(self, pid, tam, inicio):
        self.pid = pid
        self.tamano = tam
        self.inicio = inicio
        self.proceso_asignado = None
        self.frag_interna = 0 

    def asignar(self, proceso):
        self.proceso_asignado = proceso
        # CORRECCION: Calcular Fragmentación Interna.
        self.frag_interna = self.tamano - proceso.memoria_req 
        proceso.particion_id = self.pid

    def liberar(self):
        if self.proceso_asignado:
            self.proceso_asignado.particion_id = None
        self.proceso_asignado = None
        self.frag_interna = 0

# ==============================================================================
# MOTOR DE SIMULACIÓN
# ==============================================================================

def simular():
    # Inicializar particiones físicas
    particiones = [Particion(p['id'], p['tam'], p['inicio']) for p in PARTICIONES_REQUISITOS]
    todos_los_procesos = []
    
    # CORRECCION: Lectura de procesos (Id, tamaño, arribo, irrupción) desde archivo.
    try:
        with open('procesos.csv', mode='r', encoding='utf-8-sig') as f:
            reader = csv.DictReader(f)
            for row in reader:
                p = Proceso(row['id'], row['arribo'], row['irrupcion'], row['memoria'])
                # Validación de seguridad para evitar que se tilde con procesos imposibles
                if p.memoria_req > 250:
                    print(f"AVISO CRÍTICO: Proceso {p.pid} requiere {p.memoria_req}K (Máx disponible: 250K). DESCARTADO.")
                    continue
                todos_los_procesos.append(p)
    except FileNotFoundError:
        print("Error: No se encontró 'procesos.csv'. Asegúrese de que esté en la misma carpeta.")
        return

    # Colas de estados
    cola_nuevos = []
    cola_listos = []
    cola_suspendidos = [] # CORRECCION: Manejo de cola de Listos/Suspendidos.
    terminados = []
    ejecutando = None
    reloj = 0

    print("=== SIMULADOR DE PLANIFICACIÓN Y MEMORIA (UTN-FRRE) ===")

    # Ciclo principal del reloj
    while len(terminados) < len(todos_los_procesos):
        
        # 1. DETECTAR ARRIBOS (Nuevos procesos entran al sistema)
        for p in todos_los_procesos:
            if p.arribo == reloj:
                cola_nuevos.append(p)

        # 2. PLANIFICADOR DE LARGO PLAZO (Admission Scheduling)
        # CORRECCION: Implementación de la lógica de memoria correcta.
        # Se intenta cargar procesos de 'Nuevos' o 'Suspendidos' a 'Listos' (Memoria RAM).
        candidatos = cola_nuevos + cola_suspendidos
        
        # Ordenamos candidatos por arribo para ser justos (FIFO en admisión), aunque Best-Fit decidirá.
        candidatos.sort(key=lambda x: x.arribo)

        for p in list(candidatos):
            # Verificar Grado de Multiprogramación (Máximo 5 procesos en memoria) 
            en_memoria = len(cola_listos) + (1 if ejecutando else 0)
            
            if en_memoria < GRADO_MULTIPROGRAMACION:
                # CORRECCION: Política de asignación Best-Fit.
                # Busca la partición libre más pequeña que sea suficiente.
                mejor_particion = None
                for part in particiones:
                    if part.proceso_asignado is None and part.tamano >= p.memoria_req:
                        if mejor_particion is None or part.tamano < mejor_particion.tamano:
                            mejor_particion = part
                
                if mejor_particion:
                    # Asignación exitosa: Pasa a LISTO (RAM)
                    mejor_particion.asignar(p)
                    p.estado = "Listo"
                    cola_listos.append(p)
                    # Quitamos de las otras colas
                    if p in cola_nuevos: cola_nuevos.remove(p)
                    if p in cola_suspendidos: cola_suspendidos.remove(p)
                
                elif p.estado == "Nuevo":
                    # Si es nuevo y no hay lugar físico -> LISTO/SUSPENDIDO
                    p.estado = "Listo/Suspendido"
                    if p not in cola_suspendidos: cola_suspendidos.append(p)
                    if p in cola_nuevos: cola_nuevos.remove(p)
            
            elif p.estado == "Nuevo":
                 # Si supera el grado de multiprogramación -> LISTO/SUSPENDIDO
                p.estado = "Listo/Suspendido"
                if p not in cola_suspendidos: cola_suspendidos.append(p)
                if p in cola_nuevos: cola_nuevos.remove(p)

        # 3. PLANIFICADOR DE CORTO PLAZO (CPU Scheduling)
        # CORRECCION: Algoritmo SRTF (Shortest Remaining Time First).
        if cola_listos:
            # Ordenamos por Tiempo Restante (SRTF)
            cola_listos.sort(key=lambda x: x.tiempo_restante)
            
            if ejecutando:
                # Lógica de APROPIACIÓN (Preemption):
                # Si el proceso en cola es más corto que el actual, lo expulsa.
                if cola_listos[0].tiempo_restante < ejecutando.tiempo_restante:
                    ejecutando.estado = "Listo"
                    cola_listos.append(ejecutando) # Vuelve a cola de listos
                    ejecutando = cola_listos.pop(0) # Entra el nuevo
                    ejecutando.estado = "Ejecución"
            else:
                # Si CPU libre, entra el más corto
                ejecutando = cola_listos.pop(0)
                ejecutando.estado = "Ejecución"

        # 4. CÁLCULO DE ESTADÍSTICAS EN TIEMPO REAL (Para mostrar tabla detallada)
        for p in todos_los_procesos:
            if p.estado == "Terminado":
                continue
            
            # Si el proceso ya llegó, calculamos retorno actual
            if p.arribo <= reloj:
                p.tiempo_retorno = (reloj - p.arribo) + 1 # +1 porque estamos corriendo el segundo actual
            
            # Si está esperando (en RAM o Disco), suma espera
            if p.estado in ["Listo", "Listo/Suspendido"]:
                p.tiempo_espera += 1

        # 5. SALIDAS POR PANTALLA (REPORTES INSTANTÁNEOS)
        # CORRECCION: Presentación de salida ante eventos.
        print(f"\n" + "="*80)
        print(f" [ RELOJ: {reloj} ] PROCESADOR: {ejecutando.pid if ejecutando else 'IDLE'}")
        print("="*80)
        
        # CORRECCION: Tabla de Particiones con Fragmentación Interna.
        print(f"{'PARTICIÓN':<10} {'DIR_BASE':<10} {'TAMAÑO':<8} {'PROCESO':<8} {'FRAG. INT.':<12}")
        print("-" * 55)
        for pt in particiones:
            pid_str = pt.proceso_asignado.pid if pt.proceso_asignado else "LIBRE"
            print(f"{pt.pid:<10} {pt.inicio:<10} {pt.tamano:<8} {pid_str:<8} {pt.frag_interna:<12}")

        # CORRECCION: Tabla detallada de procesos (incluye colas y estadísticas actuales)
        print(f"\n{'ID':<5} {'ESTADO ACTUAL':<18} {'ARRIBO':<6} {'IRRUP':<6} {'RESTANTE':<8} {'ESPERA':<6} {'RETORNO':<7}")
        print("-" * 70)
        for p in todos_los_procesos:
            print(f"{p.pid:<5} {p.estado:<18} {p.arribo:<6} {p.irrupcion:<6} {p.tiempo_restante:<8} {p.tiempo_espera:<6} {p.tiempo_retorno:<7}")

        # CORRECCION: Estado de colas Listos y Suspendidos.
        print(f"\n> Cola de Listos (RAM): {[p.pid for p in cola_listos]}")
        print(f"> Cola de Suspendidos (Disco): {[p.pid for p in cola_suspendidos]}")

        # 6. AVANCE DEL TIEMPO (TICK DE RELOJ)
        reloj += 1
        
        if ejecutando:
            ejecutando.tiempo_restante -= 1
            if ejecutando.tiempo_restante == 0:
                # Proceso finaliza
                ejecutando.estado = "Terminado"
                ejecutando.tiempo_fin = reloj
                
                # CORRECCION: Liberar memoria al terminar.
                for pt in particiones:
                    if pt.proceso_asignado == ejecutando:
                        pt.liberar()
                
                terminados.append(ejecutando)
                ejecutando = None

    # 7. INFORME ESTADÍSTICO FINAL
    # CORRECCION: Informe estadístico y Rendimiento.
    print("\n" + "#"*80)
    print(" INFORME ESTADÍSTICO FINAL")
    print("#"*80)
    
    total_espera = 0
    total_retorno = 0
    
    for p in terminados:
        # Recalculamos finales exactos para asegurar precisión
        p.tiempo_retorno = p.tiempo_fin - p.arribo
        p.tiempo_espera = p.tiempo_retorno - p.irrupcion
        
        total_espera += p.tiempo_espera
        total_retorno += p.tiempo_retorno
        print(f"Proceso {p.pid} -> Espera: {p.tiempo_espera} | Retorno: {p.tiempo_retorno}")
    
    cant = len(terminados)
    if cant > 0:
        print(f"\nPromedio de Espera: {total_espera/cant:.2f}")
        print(f"Promedio de Retorno: {total_retorno/cant:.2f}")
        # Rendimiento: Trabajos terminados / Unidad de tiempo
        print(f"Rendimiento del Sistema: {cant/reloj:.4f} procesos/u.t.")
    else:
        print("No se completaron procesos.")

if __name__ == "__main__":
    simular()