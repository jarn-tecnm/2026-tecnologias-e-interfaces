using Agua.Api.Models;
using Microsoft.EntityFrameworkCore;

namespace Agua.Api.Data;

public class AguaDbContext(DbContextOptions<AguaDbContext> options) : DbContext(options)
{
    public DbSet<Tarjeta> Tarjetas => Set<Tarjeta>();
    public DbSet<RegistroNivel> Niveles => Set<RegistroNivel>();
    public DbSet<Solicitud> Solicitudes => Set<Solicitud>();

    protected override void OnModelCreating(ModelBuilder model)
    {
        model.Entity<Tarjeta>().ToTable("Tarjetas");
        model.Entity<Tarjeta>().Property(x => x.Nombre).HasMaxLength(100).IsRequired();
        model.Entity<RegistroNivel>().ToTable("Nivel");
        model.Entity<RegistroNivel>().Property(x => x.Nivel).HasPrecision(5, 2);
        model.Entity<RegistroNivel>().Property(x => x.FechaHora).HasPrecision(3)
            // SQL datetime2 no conserva zona; se almacenan y devuelven fechas UTC.
            .HasConversion(x => x, x => DateTime.SpecifyKind(x, DateTimeKind.Utc));
        model.Entity<RegistroNivel>().HasOne<Tarjeta>().WithMany()
            .HasForeignKey(x => x.IdTarjeta).OnDelete(DeleteBehavior.Restrict);
        model.Entity<Solicitud>().ToTable("Solicitudes");
        model.Entity<Solicitud>().HasOne<Tarjeta>().WithMany()
            .HasForeignKey(x => x.IdTarjeta).OnDelete(DeleteBehavior.Restrict);
    }
}
