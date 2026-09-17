import sys
import gi
gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, GLib
import mido

class SensorBarWidget(Gtk.Box):
    def __init__(self, sensor_num):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=6)

        title = Gtk.Label(label=f"<b>SENSOR {sensor_num}</b>")
        title.set_use_markup(True)
        self.append(title)

        self.lbl_raw = Gtk.Label(label="Raw: 0")
        self.append(self.lbl_raw)
        self.prog_raw = Gtk.ProgressBar()
        self.prog_raw.set_fraction(0.0)
        self.append(self.prog_raw)

        self.lbl_line = Gtk.Label(label="Línea: 0%")
        self.append(self.lbl_line)
        self.prog_line = Gtk.ProgressBar()
        self.prog_line.set_fraction(0.0)
        self.append(self.prog_line)

    def set_raw(self, cc_val):
        fraction = cc_val / 127.0
        raw_approx = int(fraction * 1023)
        self.prog_raw.set_fraction(fraction)
        self.lbl_raw.set_text(f"Raw: {raw_approx} ({cc_val})")

    def set_line(self, cc_val):
        fraction = cc_val / 127.0
        percentage = int(fraction * 100)
        self.prog_line.set_fraction(fraction)
        self.lbl_line.set_text(f"Línea: {percentage}%")


class MidiSensorApp(Gtk.ApplicationWindow):
    def __init__(self, app):
        super().__init__(application=app, title="Monitor MIDI - Seguidor de Línea")
        self.set_default_size(800, 300)

        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        main_box.set_margin_top(16)
        main_box.set_margin_bottom(16)
        main_box.set_margin_start(16)
        main_box.set_margin_end(16)
        self.set_child(main_box)

        self.status_label = Gtk.Label(label="Buscando 'MidiStomp'...")
        main_box.append(self.status_label)

        grid = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        grid.set_homogeneous(True)
        main_box.append(grid)

        self.sensors = []
        for i in range(1, 6):
            s_widget = SensorBarWidget(i)
            grid.append(s_widget)
            self.sensors.append(s_widget)

        self.midi_port = None
        self.connect_midi()

        GLib.timeout_add(10, self.check_midi_messages)

    def connect_midi(self):
        try:
            inputs = mido.get_input_names()

            # Filtra descartando "Midi Through" y buscando prioritariamente "MidiStomp"
            stomp_ports = [p for p in inputs if "midistomp" in p.lower()]

            if stomp_ports:
                port_name = stomp_ports[0]
                self.midi_port = mido.open_input(port_name)
                self.status_label.set_text(f"<b>Conectado a:</b> {port_name}")
                self.status_label.set_use_markup(True)
            else:
                # Si no encuentra 'MidiStomp' explícitamente, busca otro puerto USB que NO sea Midi Through
                valid_ports = [p for p in inputs if "through" not in p.lower()]
                if valid_ports:
                    port_name = valid_ports[0]
                    self.midi_port = mido.open_input(port_name)
                    self.status_label.set_text(f"<b>Conectado a:</b> {port_name}")
                    self.status_label.set_use_markup(True)
                else:
                    self.status_label.set_text("Buscando 'MidiStomp' (Descartado Midi Through)...")
        except Exception as e:
            self.status_label.set_text(f"Error al abrir puerto MIDI: {str(e)}")

    def check_midi_messages(self):
        if not self.midi_port:
            self.connect_midi()
            return True

        for msg in self.midi_port.iter_pending():
            if msg.type == 'control_change':
                cc = msg.control
                val = msg.value

                if 1 <= cc <= 5:
                    self.sensors[cc - 1].set_raw(val)
                elif 6 <= cc <= 10:
                    self.sensors[cc - 6].set_line(val)

        return True


def on_activate(app):
    win = MidiSensorApp(app)
    win.present()

if __name__ == "__main__":
    app = Gtk.Application(application_id="org.digispark.midimonitor")
    app.connect('activate', on_activate)
    app.run(sys.argv)
