import sys
import gi

gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, GLib
import mido


class SensorBarWidget(Gtk.Box):
    def __init__(self, sensor_num):
        super().__init__(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=4
        )

        # =====================================================
        # TITULO
        # =====================================================

        title = Gtk.Label(label=f"<b>SENSOR {sensor_num}</b>")
        title.set_use_markup(True)
        self.append(title)

        # =====================================================
        # RAW
        # =====================================================

        self.lbl_raw = Gtk.Label(label="Raw: 0")
        self.append(self.lbl_raw)

        self.prog_raw = Gtk.ProgressBar()
        self.prog_raw.set_fraction(0.0)
        self.append(self.prog_raw)

        # =====================================================
        # LINEA / NORMALIZADO
        # =====================================================

        self.lbl_line = Gtk.Label(label="Línea: 0%")
        self.append(self.lbl_line)

        self.prog_line = Gtk.ProgressBar()
        self.prog_line.set_fraction(0.0)
        self.append(self.prog_line)

        # =====================================================
        # MINIMO DE CALIBRACION
        # =====================================================

        self.lbl_min = Gtk.Label(label="MIN: 0")
        self.append(self.lbl_min)

        self.prog_min = Gtk.ProgressBar()
        self.prog_min.set_fraction(0.0)
        self.append(self.prog_min)

        # =====================================================
        # MAXIMO DE CALIBRACION
        # =====================================================

        self.lbl_max = Gtk.Label(label="MAX: 0")
        self.append(self.lbl_max)

        self.prog_max = Gtk.ProgressBar()
        self.prog_max.set_fraction(0.0)
        self.append(self.prog_max)

    # =========================================================
    # RAW
    # =========================================================

    def set_raw(self, cc_val):
        fraction = cc_val / 127.0
        raw_approx = int(fraction * 1023)

        self.prog_raw.set_fraction(fraction)

        self.lbl_raw.set_text(
            f"Raw: {raw_approx} ({cc_val})"
        )

    # =========================================================
    # LINEA / NORMALIZADO
    # =========================================================

    def set_line(self, cc_val):
        fraction = cc_val / 127.0
        percentage = int(fraction * 100)

        self.prog_line.set_fraction(fraction)

        self.lbl_line.set_text(
            f"Línea: {percentage}%"
        )

    # =========================================================
    # MIN
    # =========================================================

    def set_min(self, cc_val):
        fraction = cc_val / 127.0
        min_approx = int(fraction * 1023)

        self.prog_min.set_fraction(fraction)

        self.lbl_min.set_text(
            f"MIN: {min_approx} ({cc_val})"
        )

    # =========================================================
    # MAX
    # =========================================================

    def set_max(self, cc_val):
        fraction = cc_val / 127.0
        max_approx = int(fraction * 1023)

        self.prog_max.set_fraction(fraction)

        self.lbl_max.set_text(
            f"MAX: {max_approx} ({cc_val})"
        )


class MidiSensorApp(Gtk.ApplicationWindow):

    def __init__(self, app):

        super().__init__(
            application=app,
            title="Monitor MIDI - Seguidor de Línea"
        )

        self.set_default_size(900, 500)

        # =====================================================
        # CONTENEDOR PRINCIPAL
        # =====================================================

        main_box = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=12
        )

        main_box.set_margin_top(16)
        main_box.set_margin_bottom(16)
        main_box.set_margin_start(16)
        main_box.set_margin_end(16)

        self.set_child(main_box)

        # =====================================================
        # ESTADO MIDI
        # =====================================================

        self.status_label = Gtk.Label(
            label="Buscando 'MidiStomp'..."
        )

        main_box.append(self.status_label)

        # =====================================================
        # CONTENEDOR DE SENSORES
        # =====================================================

        grid = Gtk.Box(
            orientation=Gtk.Orientation.HORIZONTAL,
            spacing=12
        )

        grid.set_homogeneous(True)

        main_box.append(grid)

        # =====================================================
        # CREAR LOS 5 SENSORES
        # =====================================================

        self.sensors = []

        for i in range(1, 6):

            sensor = SensorBarWidget(i)

            grid.append(sensor)
            self.sensors.append(sensor)

        # =====================================================
        # MIDI
        # =====================================================

        self.midi_port = None

        # Intento inicial de conexión
        self.connect_midi()

        # -----------------------------------------------------
        # Leer MIDI cada 10 ms
        # -----------------------------------------------------

        GLib.timeout_add(
            10,
            self.check_midi_messages
        )

        # -----------------------------------------------------
        # Buscar reconexión cada 500 ms
        # -----------------------------------------------------

        GLib.timeout_add(
            500,
            self.reconnect_midi
        )

    # =========================================================
    # CONECTAR MIDI
    # =========================================================

    def connect_midi(self):

        # Si ya estamos conectados, no hacer nada
        if self.midi_port is not None:
            return True

        try:

            inputs = mido.get_input_names()

            # =================================================
            # BUSCAR MIDISTOMP
            # =================================================

            stomp_ports = [
                p for p in inputs
                if "midistomp" in p.lower()
            ]

            if stomp_ports:

                port_name = stomp_ports[0]

            else:

                # =================================================
                # SI NO HAY MIDISTOMP, BUSCAR OTRO USB MIDI
                # EXCEPTO MIDI THROUGH
                # =================================================

                valid_ports = [
                    p for p in inputs
                    if "through" not in p.lower()
                ]

                if not valid_ports:

                    self.status_label.set_text(
                        "Desconectado - buscando MidiStomp..."
                    )

                    return False

                port_name = valid_ports[0]

            # =================================================
            # ABRIR PUERTO
            # =================================================

            self.midi_port = mido.open_input(port_name)

            self.status_label.set_markup(
                f"<b>Conectado a:</b> {port_name}"
            )

            print(
                f"[MIDI] Conectado: {port_name}"
            )

            return True

        except Exception as e:

            self.midi_port = None

            self.status_label.set_text(
                "Desconectado - buscando MIDI..."
            )

            print(
                f"[MIDI] Error de conexión: {e}"
            )

            return False

    # =========================================================
    # RECONEXION AUTOMATICA
    # =========================================================

    def reconnect_midi(self):

        # Solo intentar si no tenemos puerto
        if self.midi_port is None:

            self.connect_midi()

        return True

    # =========================================================
    # CERRAR PUERTO MIDI
    # =========================================================

    def disconnect_midi(self):

        if self.midi_port is not None:

            try:
                self.midi_port.close()
            except Exception:
                pass

            self.midi_port = None

        self.status_label.set_text(
            "MIDI desconectado - reconectando..."
        )

    # =========================================================
    # LEER MENSAJES MIDI
    # =========================================================

    def check_midi_messages(self):

        # =====================================================
        # SI NO HAY CONEXION
        # =====================================================

        if self.midi_port is None:

            return True

        # =====================================================
        # INTENTAR LEER MIDI
        # =====================================================

        try:

            for msg in self.midi_port.iter_pending():

                # Solo nos interesan Control Change
                if msg.type != 'control_change':
                    continue

                cc = msg.control
                val = msg.value

                # =================================================
                # CC 1-5
                # RAW
                # =================================================

                if 1 <= cc <= 5:

                    sensor_index = cc - 1

                    self.sensors[
                        sensor_index
                    ].set_raw(val)

                # =================================================
                # CC 6-10
                # LINEA / NORMALIZADO
                # =================================================

                elif 6 <= cc <= 10:

                    sensor_index = cc - 6

                    self.sensors[
                        sensor_index
                    ].set_line(val)

                # =================================================
                # CC 11-15
                # MIN
                # =================================================

                elif 11 <= cc <= 15:

                    sensor_index = cc - 11

                    self.sensors[
                        sensor_index
                    ].set_min(val)

                # =================================================
                # CC 16-20
                # MAX
                # =================================================

                elif 16 <= cc <= 20:

                    sensor_index = cc - 16

                    self.sensors[
                        sensor_index
                    ].set_max(val)

        # =====================================================
        # ERROR / DESCONEXION
        # =====================================================

        except Exception as e:

            print(
                f"[MIDI] Dispositivo desconectado: {e}"
            )

            self.disconnect_midi()

        return True


# =============================================================
# GTK ACTIVATION
# =============================================================

def on_activate(app):

    win = MidiSensorApp(app)

    win.present()


# =============================================================
# MAIN
# =============================================================

if __name__ == "__main__":

    app = Gtk.Application(
        application_id="org.digispark.midimonitor"
    )

    app.connect(
        'activate',
        on_activate
    )

    app.run(sys.argv)
