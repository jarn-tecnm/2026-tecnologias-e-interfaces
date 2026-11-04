# Verificación de la entrega

Fecha: 4 de noviembre de 2026.

- Backend .NET 10: compilación correcta, sin errores ni advertencias.
- Frontend Vue 3: compilación de producción correcta.
- SQL Server 2025 Developer: ambos scripts ejecutados dos veces sin borrar datos. Se verificó la conexión con el usuario de aplicación y el rechazo de operaciones sin permiso o con datos inválidos.
- API y SQL reales: altas de tarjetas, niveles y solicitudes; listas, último registro, filtros y límites; valores 0 y 100, órdenes 0 y 1, tarjeta inexistente, campos omitidos y valores fuera de rango.
- SignalR real: recepción del evento de una lectura registrada por HTTP, con su Id y fecha UTC.
- Componente Vue real montado en un DOM simulado: consulta inicial, conexión a la API/SignalR reales, lectura enviada de 72.50%, cambio del porcentaje y altura CSS del agua, y envío de una solicitud mediante el botón de encendido.
- Swagger: documento generado y rutas verificadas.

La revisión visual en un navegador real no pudo ejecutarse por restricciones del entorno. La adaptación a pantallas pequeñas está implementada en CSS, pero no se verificó visualmente en un dispositivo físico.

No se probó una ESP32 ni una bomba física. El alcance del proyecto es registrar y consultar órdenes; no incluye firmware ni confirmaciones del actuador.

La copia no contiene contraseñas reales ni configuración local. Para ejecutarla en otro equipo es necesario preparar SQL Server y configurar la conexión siguiendo el README.

GitHub permitió consultar el repositorio, pero reportó permiso de escritura deshabilitado (`push: false`). Esta entrega no está subida al repositorio. El README incluye los comandos para cargarla con una cuenta autorizada.
