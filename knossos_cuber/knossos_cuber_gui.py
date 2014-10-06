from PyQt4.QtGui import *
from PyQt4.QtCore import QString
from knossos_cuber_widgets import Ui_Dialog
from knossos_cuber_widgets_log import Ui_dialog_log
from knossos_cuber import knossos_cuber, set_config_defaults, validate_config
import numpy as np


class KnossosCuberUILog(Ui_dialog_log):
    def __init__(self, w):
        Ui_dialog_log.__init__(self)
        self.setupUi(w)


class KnossosCuberUI(Ui_Dialog):
    def __init__(self, w, a):
        self.app = a
        Ui_Dialog.__init__(self)
        self.setupUi(w)
        self.push_button_choose_source_dir.clicked.connect(
            self.select_source_dir)
        self.push_button_choose_target_dir.clicked.connect(
            self.select_target_dir)
        self.push_button_path_to_openjpeg.clicked.connect(
            self.select_open_jpeg_bin_path)
        self.push_button_start_job.clicked.connect(
            self.run_cubing)
        self.config = dict()

        self.log_window = QDialog()
        self.log = KnossosCuberUILog(self.log_window)

    def update_gui_from_config(self):
        # General tab

        config = self.config

        if 'exp_name' in config:
            self.line_edit_experiment_name.setText(config['exp_name'])
        if 'source_path' in config:
            self.label_source_dir.setText(config['source_path'])
        if 'target_path' in config:
            self.label_target_dir.setText(config['target_path'])
        if 'perform_mag1_cubing' in config:
            if config['perform_mag1_cubing']:
                self.radio_button_start_from_2d_images.setChecked(True)
            else:
                self.radio_button_start_from_mag1.setChecked(True)
        if 'perform_compression' in config:
            self.check_box_compress.setChecked(config['perform_compression'])
        if 'perform_downsampling' in config:
            self.check_box_downsample.setChecked(config['perform_downsampling'])
        if 'skip_already_cubed_layers' in config:
            self.check_box_skip_already_generated.setChecked(
                config['skip_already_cubed_layers'])

        # Data tab

        if 'source_format' in config:
            self.line_edit_source_format.setText(config['source_format'])
        if 'source_dims' in config:
            if config['source_dims']:
                self.line_edit_source_dimensions_x.setText(
                    config['source_dims'][0])
                self.line_edit_source_dimensions_y.setText(
                    config['source_dims'][1])
        if 'source_dtype' in config:
            if config['source_dtype'] == np.uint8:
                idx = self.combo_box_source_datatype.findText('uint8')
                if idx == -1:
                    raise Exception('Combo box has no entry uint8.')
                self.combo_box_source_datatype.setCurrentIndex(idx)
            elif config['source_dtype'] == np.uint16:
                idx = self.combo_box_source_datatype.findText('uint16')
                if idx == -1:
                    raise Exception('Combo box has no entry uint16.')
                self.combo_box_source_datatype.setCurrentIndex(idx)
            else:
                raise Exception('Invalid source datatype.')
        if 'scaling' in config:
            if config['scaling']:
                self.line_edit_scaling_x.setText(str(config['scaling'][0]))
                self.line_edit_scaling_y.setText(str(config['scaling'][1]))
                self.line_edit_scaling_z.setText(str(config['scaling'][2]))
        if 'boundaries' in config:
            if config['boundaries']:
                self.line_edit_boundaries_x.setText(config['boundaries'][0])
                self.line_edit_boundaries_y.setText(config['boundaries'][1])
                self.line_edit_boundaries_z.setText(config['boundaries'][2])

        # Compression tab

        if 'open_jpeg_bin_path' in config:
            self.line_edit_path_to_openjpeg.setText(
                config['open_jpeg_bin_path'])
        if 'compression_algo' in config:
            if config['compression_algo'] == 'jpeg':
                idx = self.combo_box_compression_algorithm.findText('JPEG')
                if idx == -1:
                    raise Exception('Combo box has no entry JPEG.')
                self.combo_box_compression_algorithm.setCurrentIndex(idx)
            elif config['compression_algo'] == 'j2k':
                idx = self.combo_box_compression_algorithm.findText('JPEG2000')
                if idx == -1:
                    raise Exception('Combo box has no entry JPEG2000.')
                self.combo_box_compression_algorithm.setCurrentIndex(idx)
            else:
                raise Exception('Invalid compression algo.')
        if 'out_comp_quality' in config:
            self.spin_box_compression_quality.setValue(
                config['out_comp_quality'])
        if 'pre_comp_gauss_filter' in config:
            self.spin_box_double_gauss_filter.setValue(
                config['pre_comp_gauss_filter'])

        # Advanced tab

        if 'same_knossos_as_tif_stack_xy_orientation' in config:
            self.check_box_swap_axes.setChecked(
                config['same_knossos_as_tif_stack_xy_orientation'])
        if 'buffer_size_in_cubes' in config:
            self.spin_box_buffer_size_in_cubes.setValue(
                config['buffer_size_in_cubes'])
        if 'num_downsampling_cores' in config:
            self.spin_box_downsampling_workers.setValue(
                config['num_downsampling_cores'])
        if 'num_compression_cores' in config:
            self.spin_box_compression_workers.setValue(
                config['num_compression_cores'])
        if 'num_io_threads' in config:
            self.spin_box_io_workers.setValue(
                config['num_io_threads'])
        if 'cube_edge_len' in config:
            self.spin_box_cube_edge_length.setValue(config['cube_edge_len'])

    def update_config_from_gui(self):
        config = self.config

        # General tab

        config['exp_name'] = self.line_edit_experiment_name.text()
        config['source_path'] = self.label_source_dir.text()
        config['target_path'] = self.label_target_dir.text()
        config['dataset_base_path'] = self.label_target_dir.text()
        config['perform_mag1_cubing'] = \
            self.radio_button_start_from_2d_images.isChecked()
        config['perform_compression'] = self.check_box_compress.isChecked()
        config['perform_downsampling'] = self.check_box_downsample.isChecked()
        config['skip_already_cubed_layers'] = \
            self.check_box_skip_already_generated.isChecked()

        # Data tab

        config['source_format'] = self.line_edit_source_format.text()
        if self.line_edit_source_dimensions_x.text() and \
                self.line_edit_source_dimensions_y.text():
            config['source_dims'] = (
                int(self.line_edit_source_dimensions_x.text()),
                int(self.line_edit_source_dimensions_y.text()),
            )
        else:
            config['source_dims'] = None
        if self.combo_box_source_datatype.currentText() == 'uint8':
            config['source_dtype'] = np.uint8
        elif self.combo_box_source_datatype.currentText() == 'uint16':
            config['source_dtype'] = np.uint16
        else:
            raise Exception('Invalid source datatype.')
        config['scaling'] = (
            float(self.line_edit_scaling_x.text()),
            float(self.line_edit_scaling_y.text()),
            float(self.line_edit_scaling_z.text()),
        )
        if self.line_edit_boundaries_x.text() \
                and self.line_edit_boundaries_y.text() \
                and self.line_edit_boundaries_z.text():
            config['boundaries'] = (
                int(self.line_edit_boundaries_x.text()),
                int(self.line_edit_boundaries_y.text()),
                int(self.line_edit_boundaries_z.text()),
            )
        else:
            config['boundaries'] = None

        # Compression tab

        config['open_jpeg_bin_path'] = self.line_edit_path_to_openjpeg.text()
        if self.combo_box_compression_algorithm.currentText() == 'JPEG':
            config['compression_algo'] = 'jpeg'
        elif self.combo_box_compression_algorithm.currentText() == 'JPEG2000':
            config['compression_algo'] = 'j2k'
        else:
            raise Exception('Invalid compression algo.')
        config['out_comp_quality'] = self.spin_box_compression_quality.value()
        config['pre_comp_gauss_filter'] = \
            self.spin_box_double_gauss_filter.value()

        # Advanced tab

        if self.check_box_swap_axes.isChecked():
            config['same_knossos_as_tif_stack_xy_orientation'] = True
        else:
            config['same_knossos_as_tif_stack_xy_orientation'] = False
        config['buffer_size_in_cubes'] = \
            self.spin_box_buffer_size_in_cubes.value()
        config['num_downsampling_cores'] = \
            self.spin_box_downsampling_workers.value()
        config['num_compression_cores'] = \
            self.spin_box_compression_workers.value()
        config['num_io_threads'] = \
            self.spin_box_io_workers.value()
        config['cube_edge_len'] = \
            self.spin_box_cube_edge_length.value()

        for k, v in config.iteritems():
            if isinstance(v, QString):
                config[k] = str(v)

        return config

    def select_source_dir(self):
        self.label_source_dir.setText(QFileDialog.getExistingDirectory(
            options=QFileDialog.ShowDirsOnly))

    def select_target_dir(self):
        self.label_target_dir.setText(QFileDialog.getExistingDirectory(
            options=QFileDialog.ShowDirsOnly))

    def select_open_jpeg_bin_path(self):
        self.line_edit_path_to_openjpeg.setText(QFileDialog.getOpenFileName())

    def run_cubing(self):
        self.log_window.show()
        self.log.plain_text_edit_log.setPlainText('')

        def log_fn(text):
            self.log.plain_text_edit_log.appendPlainText(text + '\n')
            self.app.processEvents()

        config = self.update_config_from_gui()
        validate_config(config)

        knossos_cuber(config, log_fn)


if __name__ == "__main__":
    import sys
    app = QApplication(sys.argv)
    window = QDialog()
    ui = KnossosCuberUI(window, app)

    default_config = set_config_defaults()
    ui.config = default_config
    ui.update_gui_from_config()

    window.show()
    sys.exit(app.exec_())