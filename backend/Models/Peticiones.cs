using System.ComponentModel.DataAnnotations;

namespace Agua.Api.Models;

public class CrearTarjeta
{
    [Required, StringLength(100)]
    public string Nombre { get; set; } = "";
}

public class CrearNivel
{
    [Required, Range(typeof(decimal), "0", "100")]
    public decimal? Nivel { get; set; }
    [Range(1, int.MaxValue)]
    public int IdTarjeta { get; set; }
}

public class CrearSolicitud
{
    [Required, Range(0, 1)]
    public int? IdTipoOrden { get; set; }
    [Range(1, int.MaxValue)]
    public int IdTarjeta { get; set; }
}
