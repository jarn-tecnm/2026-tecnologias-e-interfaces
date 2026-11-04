-- Modo SQLCMD: proporcionar AppPassword (no guardar contraseñas reales en Git).
-- Requiere SQL Server con autenticación mixta y una cuenta administradora.
USE master;
GO
IF SUSER_ID(N'tecnm_agua_app') IS NULL
BEGIN
    DECLARE @Password nvarchar(128) = N'$(AppPassword)';
    IF LEN(@Password) < 12 OR @Password = N'CAMBIAR_CONTRASENA'
        THROW 50001, 'Proporciona AppPassword con al menos 12 caracteres.', 1;
    DECLARE @Sql nvarchar(max) = N'CREATE LOGIN tecnm_agua_app WITH PASSWORD = '
        + QUOTENAME(@Password, CHAR(39)) + N', CHECK_POLICY = ON;';
    EXEC sys.sp_executesql @Sql;
END;
GO
USE TecNM_Agua;
GO
IF USER_ID(N'tecnm_agua_app') IS NULL
    CREATE USER tecnm_agua_app FOR LOGIN tecnm_agua_app;
GRANT SELECT, INSERT ON dbo.Tarjetas TO tecnm_agua_app;
GRANT SELECT, INSERT ON dbo.Nivel TO tecnm_agua_app;
GRANT SELECT, INSERT ON dbo.Solicitudes TO tecnm_agua_app;
GO
