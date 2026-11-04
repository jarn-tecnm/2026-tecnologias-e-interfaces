-- Ejecutar con una cuenta administradora en SQL Server / SSMS.
USE master;
GO
IF DB_ID(N'TecNM_Agua') IS NULL
    CREATE DATABASE TecNM_Agua;
GO
USE TecNM_Agua;
GO
IF OBJECT_ID(N'dbo.Tarjetas', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Tarjetas (
        Id int IDENTITY(1,1) CONSTRAINT PK_Tarjetas PRIMARY KEY,
        Nombre nvarchar(100) NOT NULL,
        CONSTRAINT CK_Tarjetas_Nombre CHECK (LEN(LTRIM(RTRIM(Nombre))) > 0)
    );
END;
IF OBJECT_ID(N'dbo.Nivel', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Nivel (
        Id int IDENTITY(1,1) CONSTRAINT PK_Nivel PRIMARY KEY,
        Nivel decimal(5,2) NOT NULL,
        FechaHora datetime2(3) NOT NULL CONSTRAINT DF_Nivel_FechaHora DEFAULT SYSUTCDATETIME(),
        IdTarjeta int NOT NULL,
        CONSTRAINT CK_Nivel_Porcentaje CHECK (Nivel BETWEEN 0 AND 100),
        CONSTRAINT FK_Nivel_Tarjetas FOREIGN KEY (IdTarjeta) REFERENCES dbo.Tarjetas(Id)
    );
    CREATE INDEX IX_Nivel_IdTarjeta_Id ON dbo.Nivel(IdTarjeta, Id DESC);
END;
IF OBJECT_ID(N'dbo.Solicitudes', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Solicitudes (
        Id int IDENTITY(1,1) CONSTRAINT PK_Solicitudes PRIMARY KEY,
        IdTipoOrden int NOT NULL,
        IdTarjeta int NOT NULL,
        CONSTRAINT CK_Solicitudes_Orden CHECK (IdTipoOrden IN (0,1)),
        CONSTRAINT FK_Solicitudes_Tarjetas FOREIGN KEY (IdTarjeta) REFERENCES dbo.Tarjetas(Id)
    );
    CREATE INDEX IX_Solicitudes_IdTarjeta_Id ON dbo.Solicitudes(IdTarjeta, Id DESC);
END;
GO
