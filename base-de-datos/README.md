# Base de datos

Ejecutar `01-crear-base.sql` con una cuenta administradora en SQL Server Management Studio (SSMS). Después ejecutar `02-crear-usuario.sql` en **modo SQLCMD** (menú Consulta → Modo SQLCMD), añadiendo al inicio, sólo en la copia local:

```sql
:setvar AppPassword "TU_CONTRASENA_FUERTE_LOCAL"
```

Usar una contraseña propia de al menos 12 caracteres y que cumpla la política de SQL Server. No subirla a GitHub. Los scripts se pueden repetir sin borrar registros; el segundo no cambia la contraseña de un login existente. El login `tecnm_agua_app` tiene permisos de lectura e inserción sobre las tres tablas. No puede modificar el esquema ni borrar datos. SQL Server debe tener autenticación mixta habilitada para usar este login.

| Tabla | Campos |
| --- | --- |
| Tarjetas | Id int identity PK; Nombre nvarchar(100) |
| Nivel | Id int identity PK; Nivel decimal(5,2); FechaHora datetime2(3) UTC; IdTarjeta FK |
| Solicitudes | Id int identity PK; IdTipoOrden int (0 apagar, 1 encender); IdTarjeta FK |

`decimal(5,2)` permite tres dígitos enteros y dos decimales. En esta práctica representa un porcentaje; la base y la API lo restringen a 0–100. La API asigna la fecha UTC al recibir la lectura. Las relaciones impiden registrar niveles o solicitudes para tarjetas inexistentes. No se precargan datos: registrar una tarjeta desde Swagger.

“Último” significa mayor Id, es decir, el último registro insertado. Solicitudes conserva exactamente los campos solicitados, sin fecha ni confirmación de ejecución. Registrar una orden no confirma que la bomba se haya encendido.
