using System.ComponentModel.DataAnnotations;
using Agua.Api.Data;
using Agua.Api.Hubs;
using Agua.Api.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using Microsoft.EntityFrameworkCore;

namespace Agua.Api.Controllers;

[ApiController]
[Route("api")]
public class SistemaController(AguaDbContext db, IHubContext<NivelHub> hub) : ControllerBase
{
    [HttpGet("tarjetas")]
    public async Task<ActionResult<List<Tarjeta>>> Tarjetas() =>
        await db.Tarjetas.AsNoTracking().OrderBy(x => x.Id).ToListAsync();

    [HttpPost("tarjetas")]
    public async Task<ActionResult<Tarjeta>> RegistrarTarjeta(CrearTarjeta entrada)
    {
        if (string.IsNullOrWhiteSpace(entrada.Nombre))
            return BadRequest("El nombre es obligatorio.");
        var tarjeta = new Tarjeta { Nombre = entrada.Nombre.Trim() };
        db.Tarjetas.Add(tarjeta);
        await db.SaveChangesAsync();
        await hub.Clients.All.SendAsync("TarjetaRegistrada", tarjeta);
        return Created($"/api/tarjetas", tarjeta);
    }

    [HttpPost("niveles")]
    public async Task<ActionResult<RegistroNivel>> RegistrarNivel(CrearNivel entrada)
    {
        if (decimal.Round(entrada.Nivel!.Value, 2) != entrada.Nivel.Value)
            return BadRequest("El nivel admite como máximo dos decimales.");
        if (!await db.Tarjetas.AnyAsync(x => x.Id == entrada.IdTarjeta))
            return BadRequest("La tarjeta no está registrada.");
        var nivel = new RegistroNivel {
            Nivel = entrada.Nivel.Value, IdTarjeta = entrada.IdTarjeta, FechaHora = DateTime.UtcNow
        };
        db.Niveles.Add(nivel);
        await db.SaveChangesAsync();
        await hub.Clients.All.SendAsync("NivelActualizado", nivel);
        return Created("/api/niveles", nivel);
    }

    [HttpGet("niveles")]
    public async Task<ActionResult<List<RegistroNivel>>> Niveles(
        [FromQuery, Range(1, int.MaxValue)] int? idTarjeta,
        [FromQuery, Range(1, 1000)] int? limite)
    {
        var consulta = db.Niveles.AsNoTracking().AsQueryable();
        if (idTarjeta.HasValue) consulta = consulta.Where(x => x.IdTarjeta == idTarjeta);
        consulta = consulta.OrderByDescending(x => x.Id);
        if (limite.HasValue) consulta = consulta.Take(limite.Value);
        return await consulta.ToListAsync();
    }

    [HttpGet("niveles/ultimo")]
    public async Task<ActionResult<RegistroNivel>> UltimoNivel(
        [FromQuery, Range(1, int.MaxValue)] int? idTarjeta)
    {
        var consulta = db.Niveles.AsNoTracking().AsQueryable();
        if (idTarjeta.HasValue) consulta = consulta.Where(x => x.IdTarjeta == idTarjeta);
        var nivel = await consulta.OrderByDescending(x => x.Id).FirstOrDefaultAsync();
        return nivel is null ? NoContent() : Ok(nivel);
    }

    [HttpPost("solicitudes")]
    public async Task<ActionResult<Solicitud>> RegistrarSolicitud(CrearSolicitud entrada)
    {
        if (!await db.Tarjetas.AnyAsync(x => x.Id == entrada.IdTarjeta))
            return BadRequest("La tarjeta no está registrada.");
        var solicitud = new Solicitud {
            IdTipoOrden = entrada.IdTipoOrden!.Value, IdTarjeta = entrada.IdTarjeta
        };
        db.Solicitudes.Add(solicitud);
        await db.SaveChangesAsync();
        await hub.Clients.All.SendAsync("SolicitudRegistrada", solicitud);
        return Created("/api/solicitudes", solicitud);
    }

    [HttpGet("solicitudes")]
    public async Task<ActionResult<List<Solicitud>>> Solicitudes(
        [FromQuery, Range(1, int.MaxValue)] int? idTarjeta,
        [FromQuery, Range(1, 1000)] int? limite)
    {
        var consulta = db.Solicitudes.AsNoTracking().AsQueryable();
        if (idTarjeta.HasValue) consulta = consulta.Where(x => x.IdTarjeta == idTarjeta);
        consulta = consulta.OrderByDescending(x => x.Id);
        if (limite.HasValue) consulta = consulta.Take(limite.Value);
        return await consulta.ToListAsync();
    }

    [HttpGet("solicitudes/ultimo")]
    public async Task<ActionResult<Solicitud>> UltimaSolicitud(
        [FromQuery, Range(1, int.MaxValue)] int? idTarjeta)
    {
        var consulta = db.Solicitudes.AsNoTracking().AsQueryable();
        if (idTarjeta.HasValue) consulta = consulta.Where(x => x.IdTarjeta == idTarjeta);
        var solicitud = await consulta.OrderByDescending(x => x.Id).FirstOrDefaultAsync();
        return solicitud is null ? NoContent() : Ok(solicitud);
    }
}
