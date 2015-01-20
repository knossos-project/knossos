"""knossos_cuber_gui.py
"""

import sys
import argparse
from ast import literal_eval

from PyQt4.QtGui import *
# from PyQt4.QtCore import *
from knossos_cuber_widgets import Ui_Dialog
from knossos_cuber_widgets_log import Ui_dialog_log
from knossos_cuber import knossos_cuber, validate_config

from knossos_cuber import SOURCE_FORMAT_FILES
from knossos_cuber import read_config_file



class KnossosCuberUILog(Ui_dialog_log):
    """A QDialog that prints the process of cubing by `knossos_cuber()'."""

    def __init__(self, w):
        """
        Args:
            w (QDialog): A QDialog object

        Returns:
            KnossosCuberUILog
        """

        Ui_dialog_log.__init__(self)
        self.setupUi(w)



class KnossosCuberUI(Ui_Dialog):
    """A QDialog GUI to configurate `knossos_cuber'."""

    def __init__(self, w, a, config):
        """Creates a window and fills its content with the parameters found
        in `config'.

        Args:
            w (QDialog): A QDialog object
            a (QApplication): A QApplication object
            config (ObjectParser):
                An ObjectParser object that holds all values found in a
                configuration file.

        Returns:
            KnossosCuberUI
        """

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


        self.updateComboBoxSourceFormat()
        self.line_edit_source_dimensions_x.setDisabled(True)
        self.line_edit_source_dimensions_y.setDisabled(True)


        self.config = config

        self.log_window = QDialog()
        self.log = KnossosCuberUILog(self.log_window)


    def updateComboBoxSourceFormat(self):
        """Adds all file extensions found in `SOURCE_FORMAT_FILES' to a
        drop-down menu.
        """

        for key, value in SOURCE_FORMAT_FILES.iteritems():
            self.combo_box_source_format.addItem(value[-1])


    def update_gui_from_config(self):
        """Fills all fields with standard values from `self.config'."""

        config = self.config

        # Project options
        self.line_edit_experiment_name.setText(config.get('Project', 'exp_name'))
        if config.get('Project', 'source_path'):
            self.label_source_dir.setText(config.get('Project', 'source_path'))

        if config.get('Project', 'target_path'):
            self.label_target_dir.setText(config.get('Project', 'target_path'))


        # Dataset options
        if config.get('Dataset', 'source_dtype') == 'numpy.uint8':
            idx = self.combo_box_source_datatype.findText('uint8')
            if idx == -1:
                raise Exception('Combo box has no entry uint8.')
            self.combo_box_source_datatype.setCurrentIndex(idx)
        elif config.get('Dataset', 'source_dtype') == 'numpy.uint16':
            idx = self.combo_box_source_datatype.findText('uint16')
            if idx == -1:
                raise Exception('Combo box has no entry uint16.')
            self.combo_box_source_datatype.setCurrentIndex(idx)
        else:
            raise Exception('Invalid source datatype.')


        if config.get('Dataset', 'scaling'):
            scaling = literal_eval(config.get('Dataset', 'scaling'))
            self.line_edit_scaling_x.setText(str(scaling[0]))
            self.line_edit_scaling_y.setText(str(scaling[1]))
            self.line_edit_scaling_z.setText(str(scaling[2]))


        if config.get('Dataset', 'boundaries'):
            boundaries = literal_eval(config.get('Dataset', 'boundaries'))
            self.line_edit_boundaries_x.setText(str(boundaries[0]))
            self.line_edit_boundaries_y.setText(str(boundaries[1]))
            self.line_edit_boundaries_z.setText(str(boundaries[2]))


        if config.get('Dataset', 'source_dims'):
            source_dims = literal_eval(config.get('Dataset', 'source_dims'))
            self.line_edit_source_dimensions_x.setText(source_dims[0])
            self.line_edit_source_dimensions_y.setText(source_dims[1])


        self.check_box_swap_axes.setChecked(
            config.getboolean('Dataset', 'same_knossos_as_tif_stack_xy_orientation')
        )


        # Processing options
        if config.get('Processing', 'perform_mag1_cubing'):
            self.radio_button_start_from_2d_images.setChecked(True)
        else:
            self.radio_button_start_from_mag1.setChecked(True)


        self.check_box_downsample.setChecked(config.getboolean('Processing', 'perform_downsampling'))
        self.check_box_skip_already_generated.setChecked(config.getboolean('Processing', 'skip_already_cubed_layers'))
        self.spin_box_buffer_size_in_cubes.setValue(config.getint('Processing', 'buffer_size_in_cubes'))
        self.spin_box_cube_edge_length.setValue(config.getint('Processing', 'cube_edge_len'))
        self.spin_box_downsampling_workers.setValue(config.getint('Processing', 'num_downsampling_cores'))
        self.spin_box_compression_workers.setValue(config.getint('Compression', 'num_compression_cores'))
        self.spin_box_io_workers.setValue(config.getint('Processing', 'num_io_threads'))



        # Compression options
        self.check_box_compress.setChecked(config.getboolean('Compression', 'perform_compression'))


        if config.get('Compression', 'open_jpeg_bin_path'):
            self.line_edit_path_to_openjpeg.setText(config.get('Compression', 'open_jpeg_bin_path'))


        if config.get('Compression', 'compression_algo') == 'jpeg':
            idx = self.combo_box_compression_algorithm.findText('JPEG')
            if idx == -1:
                raise Exception('Combo box has no entry JPEG.')
            self.combo_box_compression_algorithm.setCurrentIndex(idx)
        elif config.get('Compression', 'compression_algo') == 'j2k':
            idx = self.combo_box_compression_algorithm.findText('JPEG2000')
            if idx == -1:
                raise Exception('Combo box has no entry JPEG2000.')
            self.combo_box_compression_algorithm.setCurrentIndex(idx)
        else:
            raise Exception('Invalid compression algo.')


        self.spin_box_compression_quality.setValue(config.getint('Compression', 'out_comp_quality'))
        self.spin_box_double_gauss_filter.setValue(config.getfloat('Compression', 'pre_comp_gauss_filter'))


    def update_config_from_gui(self):
        """Updates the configuration to the values from the GUI."""

        config = self.config

        # Project options
        config.set('Project', 'exp_name', str(self.line_edit_experiment_name.text()))
        config.set('Project', 'source_path', str(self.label_source_dir.text()))
        config.set('Project', 'target_path', str(self.label_target_dir.text()))


        # Dataset options
        if str(self.combo_box_source_datatype.currentText()) == 'uint16':
            config.set('Dataset', 'source_dtype', 'numpy.uint16')
        else:
            config.set('Dataset', 'source_dtype', 'numpy.uint8')

        scaling = (
            float(self.line_edit_scaling_x.text()),
            float(self.line_edit_scaling_y.text()),
            float(self.line_edit_scaling_z.text()),
        )
        config.set('Dataset', 'scaling', str(scaling))

        if self.line_edit_boundaries_x.text() and self.line_edit_boundaries_y.text() and self.line_edit_boundaries_z.text():
            boundaries = (
                int(self.line_edit_boundaries_x.text()),
                int(self.line_edit_boundaries_y.text()),
                int(self.line_edit_boundaries_z.text()),
            )

            config.set('Dataset', 'Boundaries', str(boundaries))
        else:
            config.set('Dataset', 'Boundaries', None)

        if self.line_edit_source_dimensions_x.text() and self.line_edit_source_dimensions_y.text():
            source_dims = (
                int(self.line_edit_source_dimensions_x.text()),
                int(self.line_edit_source_dimensions_y.text()),
            )
            config.set('Dataset', 'source_dims', str(source_dims))
        else:
            config.set('Dataset', 'source_dims', None)

        if self.check_box_swap_axes.isChecked():
            config.set('Dataset', 'same_knossos_as_tif_stack_xy_orientation', 'True')
        else:
            config.set('Dataset', 'same_knossos_as_tif_stack_xy_orientation', 'False')

        selected_format = self.combo_box_source_format.currentIndex()
        source_formats = enumerate(SOURCE_FORMAT_FILES)
        source_format = [x for x in source_formats][selected_format][1]
        # source_format = dict((v,k) for k, v in SOURCE_FORMAT_DICT.iteritems())[self.combo_box_source_format.currentIndex()]
        config.set('Dataset', 'source_format', source_format)


        # Processing options
        config.set(
            'Processing',
            'perform_mag1_cubing',
            'True' if self.radio_button_start_from_2d_images.isChecked() else 'False'
        )

        config.set(
            'Processing',
            'perform_downsampling',
            'True' if self.check_box_downsample.isChecked() else 'False'
        )

        config.set(
            'Processing',
            'skip_already_cubed_layers',
            'True' if self.check_box_skip_already_generated.isChecked() else 'False'
        )

        config.set(
            'Processing',
            'buffer_size_in_cubes',
            str(self.spin_box_buffer_size_in_cubes.value())
        )

        config.set(
            'Processing',
            'cube_edge_len',
            str(self.spin_box_cube_edge_length.value())
        )

        config.set(
            'Processing',
            'num_downsampling_cores',
            str(self.spin_box_downsampling_workers.value())
        )

        config.set(
            'Processing',
            'num_compression_cores',
            str(self.spin_box_compression_workers.value())
        )

        config.set(
            'Processing',
            'num_io_threads',
            str(self.spin_box_io_workers.value())
        )


        # Compression options
        config.set(
            'Compression',
            'perform_compression',
            'True' if self.check_box_compress.isChecked() else 'False'
        )

        config.set(
            'Compression',
            'open_jpeg_bin_path',
            str(self.line_edit_path_to_openjpeg.text())
        )

        if str(self.combo_box_compression_algorithm.currentText()) == 'JPEG':
            config.set('Compression', 'compression_algo', 'jpeg')
        elif str(self.combo_box_compression_algorithm.currentText()) == 'JPEG2000':
            config.set('Compression', 'compression_algo', 'j2k')
        else:
            raise Exception('Invalid compression algo.')


        config.set(
            'Compression',
            'out_comp_quality',
            str(self.spin_box_compression_quality.value())
        )

        config.set(
            'Compression',
            'pre_comp_gauss_filter',
            str(self.spin_box_double_gauss_filter.value())
        )


    def select_source_dir(self):
        self.label_source_dir.setText(
            QFileDialog.getExistingDirectory(options=QFileDialog.ShowDirsOnly)
        )


    def select_target_dir(self):
        self.label_target_dir.setText(
            QFileDialog.getExistingDirectory(options=QFileDialog.ShowDirsOnly)
        )


    def select_open_jpeg_bin_path(self):
        self.line_edit_path_to_openjpeg.setText(QFileDialog.getOpenFileName())


    def run_cubing(self):
        """Opens a window for logging, checks whether the configuration is sane
        and finally runs `knossos_cuber()' with this configuration.
        """

        self.log_window.show()
        self.log.plain_text_edit_log.setPlainText('')

        def log_fn(text):
            """A function that pipes input text to the logging window.

            Args:
                text (str): Any text that is printed during cubing.
            """

            self.log.plain_text_edit_log.appendPlainText(text + '\n')
            self.app.processEvents()


        self.update_config_from_gui()

        if validate_config(self.config):
            knossos_cuber(self.config, log_fn)


if __name__ == '__main__':
    PARSER = argparse.ArgumentParser()
    PARSER.add_argument(
        '--config', '-c',
        help="A configuration file. If no file is specified, "
             "`config.ini' from knossos_cuber's installation directory is used.",
        default='config.ini'
    )

    ARGS = PARSER.parse_args()

    if ARGS.config:
        CONFIG = read_config_file(ARGS.config)
    else:
        CONFIG = read_config_file('config.ini')

    APP = QApplication(sys.argv)
    WINDOW = QDialog()
    UI = KnossosCuberUI(WINDOW, APP, CONFIG)
    UI.update_gui_from_config()

    WINDOW.show()

    sys.exit(APP.exec_())
