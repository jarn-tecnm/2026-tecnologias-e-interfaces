# TecNM Informática · Nivel de agua

Práctica de Tecnologías e Interfaces de Computadoras: una ESP32 envía lecturas por HTTP; la API las guarda en SQL Server y avisa mediante SignalR a una pantalla Vue 3.

## Estructura

- `base-de-datos/`: scripts de tablas, relaciones y usuario SQL.
- `backend/`: API REST con C#, .NET 10, EF Core, Swagger y SignalR. Un solo controlador para el sistema.
- `frontend/`: Vue 3 y Vite. Una pantalla (`src/App.vue`), sin rutas ni login.
- `frontend/pruebas/integracion.mjs`: prueba real de API y SignalR, que inserta registros de prueba.

## 1. Base de datos

Instalar SQL Server (Express o Developer sirve para la práctica) y SSMS. Ejecutar los dos scripts en el orden indicado en [la documentación de base de datos](base-de-datos/README.md). No se crean tablas automáticamente desde EF: el script es la fuente del esquema.

## 2. Backend

Instalar el **SDK de .NET 10**. En una terminal:

```powershell
cd backend
dotnet restore
```

La configuración incluida usa autenticación de Windows contra `localhost`. Para usar el usuario SQL del script, crear `backend/appsettings.Local.json` (ignorado por Git) con:

```json
{
  "ConnectionStrings": {
    "Agua": "Server=localhost;Database=TecNM_Agua;User Id=tecnm_agua_app;Password=TU_CONTRASENA_LOCAL;Encrypt=True;TrustServerCertificate=True"
  }
}
```

Reemplazar servidor y contraseña por los de tu equipo. Para SQL Express, el servidor puede ser `localhost\\SQLEXPRESS` dentro del JSON. `TrustServerCertificate=True` facilita la práctica local con el certificado de SQL Server. También se puede configurar la variable de entorno `ConnectionStrings__Agua`.

```powershell
dotnet run
```

Swagger: <http://localhost:5001/swagger>. El perfil escucha en `0.0.0.0:5001` para admitir peticiones de la ESP32 dentro de la misma red. En la ESP32 usar `http://IP_DE_LA_LAPTOP:5001`, no `localhost`. Permitir el puerto TCP 5001 en el firewall de la red privada si hace falta. La ausencia de autenticación está pensada para esta práctica en una red controlada.

## 3. Frontend

Instalar Node.js 22.12 o superior (también funciona Node 24). Abrir otra terminal:

```powershell
cd frontend
npm ci
npm run dev
```

Abrir <http://localhost:5173>. Vite redirige `/api` y `/hubs` a la API en el puerto 5001. No es necesario editar direcciones para la práctica, incluso al abrir la página con la IP de la laptop desde un celular en la misma red; permitir también el puerto 5173.

La pantalla consulta el último nivel y las tablas al abrir, conecta SignalR y vuelve a consultar para recuperar cualquier lectura recibida durante la conexión. Cada aviso de SignalR actualiza los datos; al reconectar también se sincronizan. Si no existen lecturas se muestra “Sin registros”, sin inventar un 0%. El tanque representa el último registro entre **todas las tarjetas** e identifica la tarjeta correspondiente. Las tablas muestran las diez inserciones más recientes entre todas las tarjetas.

Para publicar por separado: copiar `.env.example` a `.env.local`, asignar `VITE_API_URL=https://URL_DE_LA_API` y ejecutar `npm run build`. Agregar el origen exacto del frontend a `FrontendOrigins` del backend. El servidor del frontend debe servir `dist/`; `npm run preview` sólo sirve para revisar la compilación local. El alojamiento de la API debe permitir WebSockets o los transportes alternativos de SignalR.

## Endpoints

Todos están en `/api`. Las respuestas usan nombres JSON en camelCase.

| Método | Ruta | Función |
| --- | --- | --- |
| POST | `/tarjetas` | Registrar tarjeta |
| GET | `/tarjetas` | Consultar tarjetas |
| POST | `/niveles` | Registrar lectura y publicar `NivelActualizado` |
| GET | `/niveles` | Consultar todas las lecturas |
| GET | `/niveles/ultimo` | Última lectura insertada |
| POST | `/solicitudes` | Registrar solicitud de encendido/apagado |
| GET | `/solicitudes` | Consultar todas las solicitudes |
| GET | `/solicitudes/ultimo` | Última solicitud insertada |

Las consultas de niveles y solicitudes admiten `?idTarjeta=1` para filtrar. Las consultas de listas también admiten `&limite=10` (1–1000); sin límite devuelven todos los registros. Las listas se ordenan por Id descendente. Los endpoints de último registro responden **204** si no hay datos; los POST válidos responden **201** y los datos inválidos, **400**.

Hub: `/hubs/nivel`. Eventos: `NivelActualizado`, `SolicitudRegistrada` y `TarjetaRegistrada`; cada uno transporta el registro guardado completo. Los eventos se envían después de guardar en SQL Server. Las inserciones hechas directamente en SQL no generan eventos: utilizar la API.

## Prueba manual integrada

1. En Swagger, ejecutar `POST /api/tarjetas`:

```json
{ "nombre": "ESP32 Tanque Edificio 1" }
```

2. Usar el **Id devuelto** (puede ser distinto de 1) en `POST /api/niveles`:

```json
{ "nivel": 65.25, "idTarjeta": 1 }
```

3. Abrir la pantalla Vue en dos pestañas. Enviar otro nivel desde Swagger: ambas deben cambiar sin recargar y mostrar el registro en la tabla.
4. Elegir una tarjeta y presionar **Encender bomba** o **Apagar bomba**. Revisar la solicitud en Swagger y en la tabla.
5. La ESP32 puede consultar `GET /api/solicitudes/ultimo?idTarjeta=1`. `idTipoOrden: 1` significa encender; `0`, apagar. Debe conservar el último Id procesado para evitar repetir órdenes. Este proyecto no incluye firmware ni confirma el estado real de la bomba. Consultar sólo la última orden sirve para esta práctica; no constituye una cola con confirmaciones.
6. Detener y reiniciar la API. La página reconecta automáticamente y recupera los datos guardados. Las fechas se almacenan en UTC y se presentan en la hora local del navegador.

## Comprobaciones

```powershell
dotnet build backend
cd frontend
npm ci
npm run build
# Requiere SQL preparado y la API ejecutándose. Inserta una tarjeta y registros de prueba:
node pruebas/integracion.mjs
```

La prueba verifica inserciones, consultas, filtros, validaciones y recepción real del aviso SignalR. No borra registros automáticamente. Para usar otra API: `node pruebas/integracion.mjs http://IP:5001`.

## Subir esta copia a GitHub

Si descargaste el ZIP porque la conexión sólo tenía lectura, extraerlo y abrir una terminal dentro de la carpeta del proyecto. Para el repositorio vacío indicado:

```powershell
git init
git add .
git commit -m "Crear sistema de nivel de agua con SQL Server, API y Vue"
git branch -M main
git remote add origin https://github.com/jarn-tecnm/2026-tecnologias-e-interfaces.git
git push -u origin main
```

Se requiere una cuenta con permiso de escritura. Si el repositorio ya contiene archivos, clonarlo primero y copiar dentro los archivos del proyecto, sin reemplazar cambios ajenos. No usar `push --force`. Nunca subir `appsettings.Local.json`, contraseñas ni `node_modules`.

## Referencias técnicas

- [SignalR: cliente JavaScript](https://learn.microsoft.com/aspnet/core/signalr/javascript-client?view=aspnetcore-10.0)
- [EF Core con SQL Server](https://learn.microsoft.com/ef/core/providers/sql-server/)
- [Vue: introducción](https://vuejs.org/guide/introduction.html)
