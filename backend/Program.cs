using Agua.Api.Data;
using Agua.Api.Hubs;
using Microsoft.EntityFrameworkCore;

var builder = WebApplication.CreateBuilder(args);
builder.Configuration.AddJsonFile("appsettings.Local.json", optional: true)
    .AddEnvironmentVariables().AddCommandLine(args);
builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();
builder.Services.AddSignalR();
builder.Services.AddDbContext<AguaDbContext>(options =>
    options.UseSqlServer(builder.Configuration.GetConnectionString("Agua")));
builder.Services.AddCors(options => options.AddDefaultPolicy(policy =>
    policy.WithOrigins(builder.Configuration.GetSection("FrontendOrigins").Get<string[]>()
        ?? ["http://localhost:5173"]).AllowAnyHeader().AllowAnyMethod().AllowCredentials()));

var app = builder.Build();
app.UseSwagger();
app.UseSwaggerUI();
app.UseCors();
app.MapControllers();
app.MapHub<NivelHub>("/hubs/nivel");
app.MapGet("/", () => Results.Redirect("/swagger"));
app.Run();
