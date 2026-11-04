<script setup>
import { computed, onMounted, onBeforeUnmount, ref } from "vue";
import { HubConnectionBuilder, LogLevel } from "@microsoft/signalr";

const base = (import.meta.env.VITE_API_URL || "").replace(/\/$/, "");
const tarjetas = ref([]),
  niveles = ref([]),
  solicitudes = ref([]),
  ultimo = ref(null);
const estado = ref("Conectando"),
  error = ref(""),
  aviso = ref("");
const tarjetaOrden = ref(""),
  enviando = ref(false),
  cargando = ref(true);
const porcentaje = computed(() => Number(ultimo.value?.nivel ?? 0));
const nombreTarjeta = computed(
  () =>
    tarjetas.value.find((t) => t.id === ultimo.value?.idTarjeta)?.nombre ??
    "Sin tarjeta",
);
let cerrado = false,
  reintento,
  revision = 0,
  actualizando = false;

async function consultar(ruta) {
  const respuesta = await fetch(`${base}/api/${ruta}`);
  if (!respuesta.ok)
    throw new Error(
      "No se pudieron consultar los datos. Revisa que la API y SQL Server estén disponibles.",
    );
  return respuesta.status === 204 ? null : respuesta.json();
}

// Repetir la consulta si llega un evento mientras se descarga la información.
async function actualizar() {
  revision++;
  if (actualizando || cerrado) return;
  actualizando = true;
  try {
    let version;
    do {
      version = revision;
      const datos = await Promise.all([
        consultar("tarjetas"),
        consultar("niveles?limite=10"),
        consultar("solicitudes?limite=10"),
        consultar("niveles/ultimo"),
      ]);
      if (version === revision && !cerrado) {
        [tarjetas.value, niveles.value, solicitudes.value, ultimo.value] =
          datos;
        error.value = "";
        if (!tarjetaOrden.value && tarjetas.value.length)
          tarjetaOrden.value = tarjetas.value[0].id;
      }
    } while (version !== revision && !cerrado);
  } catch (e) {
    error.value = e.message;
  } finally {
    actualizando = false;
    cargando.value = false;
  }
}

const conexion = new HubConnectionBuilder()
  .withUrl(`${base}/hubs/nivel`)
  .withAutomaticReconnect({ nextRetryDelayInMilliseconds: () => 5000 })
  .configureLogging(LogLevel.Warning)
  .build();

conexion.on("NivelActualizado", actualizar);
conexion.on("SolicitudRegistrada", actualizar);
conexion.on("TarjetaRegistrada", actualizar);
conexion.onreconnecting(() => {
  estado.value = "Reconectando";
});
conexion.onreconnected(async () => {
  estado.value = "En vivo";
  await actualizar();
});
conexion.onclose(() => {
  if (!cerrado) {
    estado.value = "Sin conexión";
    reintento = setTimeout(conectar, 5000);
  }
});

async function conectar() {
  if (cerrado) return;
  try {
    await conexion.start();
    if (cerrado) {
      await conexion.stop();
      return;
    }
    estado.value = "En vivo";
    // Recuperar lecturas que llegaron entre la consulta inicial y la conexión.
    await actualizar();
  } catch {
    if (!cerrado) {
      estado.value = "Sin conexión";
      reintento = setTimeout(conectar, 5000);
    }
  }
}

async function enviarOrden(idTipoOrden) {
  enviando.value = true;
  aviso.value = "";
  error.value = "";
  try {
    const respuesta = await fetch(`${base}/api/solicitudes`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        idTipoOrden,
        idTarjeta: Number(tarjetaOrden.value),
      }),
    });
    if (!respuesta.ok)
      throw new Error(
        "No se pudo registrar la solicitud. Verifica la tarjeta y la conexión.",
      );
    aviso.value = `Solicitud de ${idTipoOrden === 1 ? "encendido" : "apagado"} registrada.`;
    await actualizar();
  } catch (e) {
    error.value = e.message;
  } finally {
    enviando.value = false;
  }
}

function fecha(valor) {
  return new Date(valor).toLocaleString("es-MX");
}
onMounted(async () => {
  await actualizar();
  await conectar();
});
onBeforeUnmount(() => {
  cerrado = true;
  clearTimeout(reintento);
  conexion.stop();
});
</script>

<template>
  <header class="cabecera">
    <div class="marca">
      <span class="emblema" aria-hidden="true">T</span>
      <div>
        <p class="etiqueta">INGENIERÍA EN INFORMÁTICA</p>
        <h1>TecNM Informática</h1>
      </div>
    </div>
    <span class="conexion" :class="{ vivo: estado === 'En vivo' }" role="status"
      ><i></i>{{ estado }}</span
    >
  </header>
  <main>
    <div class="titulo">
      <div>
        <p class="etiqueta">MONITOREO DEL AGUA</p>
        <h2>Nivel de Agua - Tanque Edificio 1</h2>
        <p>Consulta la última lectura y sigue los cambios del tanque.</p>
      </div>
      <button class="secundario" @click="actualizar">Actualizar datos</button>
    </div>
    <p v-if="error" role="alert" class="mensaje error">{{ error }}</p>
    <p v-if="aviso" role="status" class="mensaje aviso">{{ aviso }}</p>
    <section class="panel-principal">
      <div class="visual-tanque">
        <div class="escala" aria-hidden="true">
          <span>100%</span><span>75%</span><span>50%</span><span>25%</span
          ><span>0%</span>
        </div>
        <div
          class="tanque"
          role="meter"
          aria-label="Nivel de agua del tanque"
          aria-valuemin="0"
          aria-valuemax="100"
          :aria-valuenow="ultimo ? porcentaje : undefined"
          :aria-valuetext="ultimo ? `${porcentaje}%` : 'Sin lecturas'"
        >
          <div class="agua" :style="{ height: porcentaje + '%' }"></div>
          <div class="lineas" aria-hidden="true"></div>
          <span class="valor-tanque">{{
            ultimo ? porcentaje.toFixed(2) + "%" : "—"
          }}</span>
        </div>
      </div>
      <div class="resumen">
        <p class="etiqueta">NIVEL ACTUAL</p>
        <p class="numero">
          {{ ultimo ? porcentaje.toFixed(2) : "—" }}<span v-if="ultimo">%</span>
        </p>
        <p class="descripcion">
          {{
            cargando
              ? "Consultando lecturas…"
              : ultimo
                ? "Porcentaje de llenado del tanque"
                : "Todavía no hay lecturas registradas"
          }}
        </p>
        <dl>
          <div>
            <dt>Última lectura</dt>
            <dd>{{ ultimo ? fecha(ultimo.fechaHora) : "Sin registros" }}</dd>
          </div>
          <div>
            <dt>Tarjeta de la lectura</dt>
            <dd>
              {{ ultimo ? `${nombreTarjeta} · #${ultimo.idTarjeta}` : "—" }}
            </dd>
          </div>
        </dl>
        <div class="control">
          <label for="tarjeta">Enviar solicitud a la tarjeta</label
          ><select
            id="tarjeta"
            v-model="tarjetaOrden"
            :disabled="!tarjetas.length"
          >
            <option value="" disabled>Sin tarjetas registradas</option>
            <option v-for="t in tarjetas" :key="t.id" :value="t.id">
              {{ t.nombre }} (#{{ t.id }})
            </option>
          </select>
          <div class="botones">
            <button
              :disabled="!tarjetaOrden || enviando"
              @click="enviarOrden(1)"
            >
              Encender bomba</button
            ><button
              class="secundario"
              :disabled="!tarjetaOrden || enviando"
              @click="enviarOrden(0)"
            >
              Apagar bomba
            </button>
          </div>
          <p class="nota">
            Se registra la orden para que la ESP32 la consulte. No confirma el
            estado físico de la bomba.
          </p>
        </div>
      </div>
    </section>
    <section class="panel">
      <div class="titulo-seccion">
        <h3>Tarjetas registradas</h3>
        <span>{{ tarjetas.length }} tarjetas</span>
      </div>
      <div class="tabla">
        <table>
          <thead>
            <tr>
              <th>Id</th>
              <th>Nombre</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="t in tarjetas" :key="t.id">
              <td>#{{ t.id }}</td>
              <td>{{ t.nombre }}</td>
            </tr>
            <tr v-if="!tarjetas.length">
              <td colspan="2">
                {{
                  cargando
                    ? "Cargando…"
                    : "Registra la primera tarjeta desde Swagger."
                }}
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>
    <section class="panel">
      <div class="titulo-seccion">
        <h3>Últimos 10 niveles de agua</h3>
        <span>Más reciente primero</span>
      </div>
      <div class="tabla">
        <table>
          <thead>
            <tr>
              <th>Id</th>
              <th>Nivel (%)</th>
              <th>Fecha y hora local</th>
              <th>IdTarjeta</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="n in niveles" :key="n.id">
              <td>#{{ n.id }}</td>
              <td class="dato">{{ Number(n.nivel).toFixed(2) }}%</td>
              <td>{{ fecha(n.fechaHora) }}</td>
              <td>{{ n.idTarjeta }}</td>
            </tr>
            <tr v-if="!niveles.length">
              <td colspan="4">
                {{ cargando ? "Cargando…" : "Sin niveles registrados." }}
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>
    <section class="panel">
      <div class="titulo-seccion">
        <h3>Últimas 10 solicitudes</h3>
        <span>0 = apagar · 1 = encender</span>
      </div>
      <div class="tabla">
        <table>
          <thead>
            <tr>
              <th>Id</th>
              <th>IdTipoOrden</th>
              <th>IdTarjeta</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="s in solicitudes" :key="s.id">
              <td>#{{ s.id }}</td>
              <td>
                <span class="orden"
                  >{{ s.idTipoOrden }} ·
                  {{ s.idTipoOrden === 1 ? "Encender" : "Apagar" }}</span
                >
              </td>
              <td>{{ s.idTarjeta }}</td>
            </tr>
            <tr v-if="!solicitudes.length">
              <td colspan="3">
                {{ cargando ? "Cargando…" : "Sin solicitudes registradas." }}
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>
  </main>
  <footer>
    TecNM · Informática <span>Un recurso valioso. Un monitoreo sencillo.</span>
  </footer>
</template>
