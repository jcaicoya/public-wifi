# RUNBOOK — Public Wi-Fi Cybershow

Este documento resume la operativa del show en modo `live`. Está pensado para el equipo técnico que monta, arranca y cierra la función.

## 1. Objetivo

El show usa un router GL.iNet como punto de acceso controlado. La aplicación Qt muestra en pantalla la actividad de red, los dispositivos conectados, el portal cautivo, el mapa, el perfil de riesgo y el análisis de cifrado.

La idea es siempre la misma:

- un router dedicado;
- un móvil controlado;
- un portátil de operador;
- la aplicación en modo `live`.

## 2. Antes de abrir puertas

1. Encender el router Mango / GL.iNet GL-MT300N-V2.
2. Conectar su alimentación por USB.
3. Esperar a que el router arranque por completo.
4. Comprobar que el portátil del operador puede llegar a `http://192.168.8.1`.
5. Comprobar que el móvil de prueba está disponible y listo para conectarse.
6. Verificar que el acceso SSH al router funciona con `root@192.168.8.1`.

Si algo de esto falla, no conviene empezar el show todavía.

## 3. Red Wi-Fi del show

SSID habitual:

- `GL-MT300N-V2-28e`

La contraseña debe estar guardada en la libreta operativa o en el soporte acordado por el equipo. En algunos montajes iniciales fue `goodlife`, pero no debe darse por fija: hay que usar la que corresponda al router preparado para esa función.

## 4. Arranque del show

1. Encender router.
2. Conectar el portátil del operador a la Wi-Fi del router.
3. Abrir el panel del router en el navegador:
   - `http://192.168.8.1`
4. Conectar el móvil controlado a la misma Wi-Fi.
5. Lanzar la aplicación:

```powershell
public_wifi.exe --show --profile live --fullscreen
```

La aplicación debe abrir directamente la ejecución, no la pantalla de configuración.

## 5. Comprobaciones en vivo

### Pantalla 1: Principal

Aquí se controla el estado general.

Hay que comprobar:

- que el router aparece como listo;
- que la app reconoce el modo `LIVE`;
- que los puertos `5555`, `5556` y `8080` están preparados;
- que los contadores y estados cambian cuando hay actividad.

### Pantalla 2: Dispositivos + tráfico

En esta pantalla se ve:

- la lista de dispositivos conectados;
- el tráfico bruto del router;
- la URL del portal cautivo;
- la alerta de credenciales cuando el móvil usa el portal.

Flujo habitual:

1. Mostrar la URL del portal.
2. Hacer que el teléfono entre al portal.
3. Pedir el nombre y el correo.
4. Ver la credencial interceptada en la pantalla.

### Pantalla 3: Mapa / conexiones

Se usa para enseñar:

- las conexiones activas;
- el mapa de origen/destino;
- el dispositivo objetivo seleccionado;
- los eventos asociados a ese dispositivo.

### Pantalla 4: Perfil de riesgo

Se usa para explicar el riesgo operativo:

- puntuación;
- categorías detectadas;
- servicios observados;
- factores de riesgo;
- resumen para el operador.

### Pantalla 5: Análisis de cifrado

Se usa como momento escénico controlado.

En esta pantalla:

- el operador inicia el análisis;
- la consola escribe el flujo paso a paso;
- la secuencia termina en fallo controlado;
- el resultado deja claro que el cifrado no se rompe.

## 6. Comandos SSH útiles

Entrar al router:

```powershell
ssh root@192.168.8.1
```

Desde el router se pueden lanzar los scripts de eventos:

```bash
/root/send_traffic_events.sh 192.168.8.182 5555
/root/device_watch.sh 192.168.8.182 5556
```

Y, si hace falta, observar la actividad del sistema:

```bash
logread -f
```

## 7. Cierre del show

1. Parar la app.
2. Entrar por SSH al router si sigue activo.
3. Detener los procesos de eventos si están en ejecución.
4. Cerrar la sesión SSH.
5. Apagar el router si toca desmontaje.
6. Desconectar la alimentación USB.

## 8. Apagado recomendado

Si se está dentro de SSH:

```bash
Ctrl + C
exit
```

Si el equipo ya no se va a usar:

```bash
poweroff
```

## 9. Puntos importantes

- El modo `live` depende del router y de sus scripts.
- El modo `demo` no necesita router.
- El teléfono del show debe ser el único dispositivo controlado que genere la actividad visible.
- No hay que exponer datos reales de público ni credenciales fuera del flujo previsto.
- Si la red no está lista, es mejor retrasar el arranque que improvisar.
