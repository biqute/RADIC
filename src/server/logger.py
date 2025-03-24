import logging
from pathlib import Path

filename = "logs/logger.log"

class Logger():
    def __init__(self, 
                 filename: str = filename,
                 formatter = logging.Formatter("%(asctime)s - %(levelname)s: %(message)s", datefmt="%m/%d/%Y %H:%M:%S")
            ):
        """filename - \"logs/logger.log\""""
        
        self.logger = logging.getLogger(Logger.__name__)
        self.logger.setLevel(logging.DEBUG)

        self.check_folder(filename)
        fh = logging.FileHandler(self.check_folder(filename), "a")
        fh.setFormatter(formatter)

        self.logger.addHandler(fh)

    def check_folder(self, filename: str) -> str:
        dir_path = Path(filename).parent
        if not dir_path.exists():
            dir_path.mkdir()
        return filename