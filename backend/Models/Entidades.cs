namespace Agua.Api.Models;

public class Tarjeta
{
    public int Id { get; set; }
    public string Nombre { get; set; } = "";
}

public class RegistroNivel
{
    public int Id { get; set; }
    public decimal Nivel { get; set; }
    public DateTime FechaHora { get; set; }
    public int IdTarjeta { get; set; }
}

public class Solicitud
{
    public int Id { get; set; }
    public int IdTipoOrden { get; set; }
    public int IdTarjeta { get; set; }
}
