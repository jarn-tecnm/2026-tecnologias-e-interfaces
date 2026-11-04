import assert from "node:assert/strict";
import { HubConnectionBuilder, LogLevel } from "@microsoft/signalr";

const base = (process.argv[2] || "http://localhost:5001").replace(/\/$/, "");
async function api(ruta, body, status = 200) {
  const r = await fetch(
    `${base}/api/${ruta}`,
    body === undefined
      ? {}
      : {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(body),
        },
  );
  assert.equal(
    r.status,
    status,
    `${ruta}: ${await (r.status !== status ? r.text() : Promise.resolve(""))}`,
  );
  return r.status === 204 || r.status >= 400 ? null : r.json();
}

const conexion = new HubConnectionBuilder()
  .withUrl(`${base}/hubs/nivel`)
  .configureLogging(LogLevel.Error)
  .build();
try {
  const tarjeta = await api(
    "tarjetas",
    { nombre: `Prueba ${Date.now()}` },
    201,
  );
  assert.ok(tarjeta.id > 0);
  assert.ok((await api("tarjetas")).some((t) => t.id === tarjeta.id));
  assert.equal(
    await api(`niveles/ultimo?idTarjeta=${tarjeta.id}`, undefined, 204),
    null,
  );
  assert.equal(
    await api(`solicitudes/ultimo?idTarjeta=${tarjeta.id}`, undefined, 204),
    null,
  );
  await api("tarjetas", { nombre: " " }, 400);
  await api("niveles", { nivel: -1, idTarjeta: tarjeta.id }, 400);
  await api("niveles", { nivel: 101, idTarjeta: tarjeta.id }, 400);
  await api("niveles", { nivel: 2.123, idTarjeta: tarjeta.id }, 400);
  await api("niveles", { idTarjeta: tarjeta.id }, 400);
  await api("niveles", { nivel: 50, idTarjeta: 2147483647 }, 400);
  await api("solicitudes", { idTipoOrden: 2, idTarjeta: tarjeta.id }, 400);
  await api("solicitudes", { idTarjeta: tarjeta.id }, 400);
  await api("solicitudes", { idTipoOrden: 0, idTarjeta: 2147483647 }, 400);
  await api("niveles?limite=0", undefined, 400);
  await api("solicitudes?idTarjeta=-1", undefined, 400);
  let resolver;
  const recibido = new Promise((resolve) => {
    resolver = resolve;
  });
  conexion.on("NivelActualizado", (dato) => {
    if (dato.idTarjeta === tarjeta.id) resolver(dato);
  });
  await conexion.start();
  let timer;
  try {
    const espera = Promise.race([
      recibido,
      new Promise((_, reject) => {
        timer = setTimeout(
          () => reject(new Error("No llegó el evento SignalR")),
          10000,
        );
      }),
    ]);
    const nivel = await api(
      "niveles",
      { nivel: 65.25, idTarjeta: tarjeta.id },
      201,
    );
    const evento = await espera;
    assert.equal(evento.id, nivel.id);
    assert.equal(evento.nivel, 65.25);
    assert.ok(evento.fechaHora.endsWith("Z"), "La fecha debe indicar UTC");
    assert.equal(
      (await api(`niveles/ultimo?idTarjeta=${tarjeta.id}`)).id,
      nivel.id,
    );
    const segundo = await api(
      "niveles",
      { nivel: 0, idTarjeta: tarjeta.id },
      201,
    );
    await api("niveles", { nivel: 100, idTarjeta: tarjeta.id }, 201);
    const lista = await api(`niveles?idTarjeta=${tarjeta.id}&limite=2`);
    assert.equal(lista.length, 2);
    assert.equal(lista[1].id, segundo.id);
    assert.ok((await api("niveles")).some((n) => n.id === nivel.id));
    for (const orden of [1, 0]) {
      const solicitud = await api(
        "solicitudes",
        { idTipoOrden: orden, idTarjeta: tarjeta.id },
        201,
      );
      assert.equal(
        (await api(`solicitudes/ultimo?idTarjeta=${tarjeta.id}`)).id,
        solicitud.id,
      );
    }
    assert.equal(
      (await api(`solicitudes?idTarjeta=${tarjeta.id}&limite=1`)).length,
      1,
    );
    assert.ok(
      (await api("solicitudes")).some((s) => s.idTarjeta === tarjeta.id),
    );
    console.log(
      `OK: API, SQL Server y SignalR integrados. Tarjeta de prueba: ${tarjeta.id}`,
    );
  } finally {
    clearTimeout(timer);
  }
} finally {
  await conexion.stop();
}
