import random
import os
from datetime import date

TIPS_FILE = "tips.txt"
FAV_FILE = "favoritos.txt"
HISTORIAL_FILE = "historial.txt"
ULTIMO_TIP_FILE = "ultimo_tip.txt"

def cargar_tips():
    if not os.path.exists(TIPS_FILE):
        return []
    with open(TIPS_FILE, "r", encoding="utf-8") as f:
        return [line.strip() for line in f if line.strip()]

def guardar_favorito(tip):
    with open(FAV_FILE, "a", encoding="utf-8") as f:
        f.write(tip + "\n")
    print("Consejo guardado como favorito.")

def guardar_historial(tip):
    with open(HISTORIAL_FILE, "a", encoding="utf-8") as f:
        f.write(tip + "\n")

def leer_ultimo_tip():
    if not os.path.exists(ULTIMO_TIP_FILE):
        return None, None
    with open(ULTIMO_TIP_FILE, "r", encoding="utf-8") as f:
        lineas = f.readlines()
        if len(lineas) >= 2:
            fecha_guardada = lineas[0].strip()
            tip_guardado = lineas[1].strip()
            return fecha_guardada, tip_guardado
    return None, None

def guardar_ultimo_tip(tip):
    with open(ULTIMO_TIP_FILE, "w", encoding="utf-8") as f:
        f.write(f"{date.today()}\n{tip}\n")

def obtener_tip_del_dia(tips):
    fecha_hoy = str(date.today())
    fecha_guardada, tip_guardado = leer_ultimo_tip()

    if fecha_guardada == fecha_hoy and tip_guardado:
        return tip_guardado
    else:
        tip = random.choice(tips)
        guardar_ultimo_tip(tip)
        guardar_historial(tip)
        return tip

def cambiar_tip_del_dia(tips, tip_actual):
    opciones_disponibles = [t for t in tips if t != tip_actual]
    if not opciones_disponibles:
        print("No hay más consejos disponibles para hoy.")
        return tip_actual
    nuevo_tip = random.choice(opciones_disponibles)
    guardar_ultimo_tip(nuevo_tip)
    guardar_historial(nuevo_tip)
    print(f"\nConsejo cambiado por:\n {nuevo_tip}\n")
    return nuevo_tip

def ver_favoritos():
    print("\nTus consejos favoritos:")
    if not os.path.exists(FAV_FILE):
        print("Aún no hay favoritos guardados.")
        return
    with open(FAV_FILE, "r", encoding="utf-8") as f:
        for line in f:
            print(f"✔ {line.strip()}")

def ver_historial():
    print("\nHistorial de consejos vistos:")
    if not os.path.exists(HISTORIAL_FILE):
        print("No hay historial aún.")
        return
    with open(HISTORIAL_FILE, "r", encoding="utf-8") as f:
        for line in f:
            print(f"- {line.strip()}")

def mostrar_menu():
    print("\nMenú Green Tips Diario")
    print("1. Mostrar el consejo verde del día")
    print("2. Ver consejos favoritos")
    print("3. Ver historial de tips vistos")
    print("4. Salir")

def main():
    tips = cargar_tips()
    if not tips:
        print("No se encontraron consejos en tips.txt")
        return

    tip_del_dia = obtener_tip_del_dia(tips)

    while True:
        mostrar_menu()
        opcion = input("Elige una opción (1-4): ")

        if opcion == "1":
            while True:
                print(f"\nConsejo verde del día:\n{tip_del_dia}\n")
                print("¿Querés hacer algo con este tip?")
                print("a) Guardar como favorito")
                print("b) Cambiar por otro tip")
                print("c) Volver al menú principal")
                sub_op = input("Elegí una opción (a/b/c): ")
                if sub_op == "a":
                    guardar_favorito(tip_del_dia)
                elif sub_op == "b":
                    tip_del_dia = cambiar_tip_del_dia(tips, tip_del_dia)
                elif sub_op == "c":
                    break
                else:
                    print("Opción no válida.")
        elif opcion == "2":
            ver_favoritos()
        elif opcion == "3":
            ver_historial()
        elif opcion == "4":
            print("¡Gracias por usar Green Tips Diarios!")
            break
        else:
            print("Opción inválida. Intentá de nuevo.")

if __name__ == "__main__":
    main()