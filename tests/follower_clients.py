import argparse
import queue
import subprocess
import sys
import threading

from pathlib import Path

class OdamexProcess:
    """This class takes ownership of, wraps, and manages an Odamex process.
    The process must have been launched beforehand with subprocess.  This class
    provides APIs to poll for its continued operation as well as provide
    console input and obtain console output."""

    def __init__(self, process: subprocess.Popen):
        self._process       = process
        self._in_queue      = queue.SimpleQueue()
        self._out_queue     = queue.SimpleQueue()
        self._is_shutdown   = threading.Event()

        self._poll_result = None

        self._in_thread  = threading.Thread(target=self._forward_input)
        self._out_thread = threading.Thread(target=self._forward_output)

        self._in_thread.start()
        self._out_thread.start()

    @property
    def pid(self) -> int:
        return self._process.pid

    def poll(self) -> int | None:
        """Polls the Odamex process for completion.  Returns None if the process is still
        running, or the process's exit code if it has exited."""

        poll_result = self._process.poll()
        if poll_result is not None and self._poll_result is None:
            self._poll_result = poll_result

            self._is_shutdown.set()
            self._in_queue.put('\n')    # Force a wakeup.
            self._in_thread.join()
            self._out_thread.join()

        return poll_result

    def _forward_input(self):
        """Internal thread main function for sending input to the process."""
        while True:
            input_line = self._in_queue.get()
            if self._is_shutdown.is_set():
                break
            self._process.stdin.write(input_line)
            self._process.stdin.flush()

    def _forward_output(self):
        """Internal thread main function for obtaining output from the process."""
        for line in self._process.stdout:
            self._out_queue.put(line)

    def send_input(self, input_line: str):
        """Send the given input string to the Odamex process's console.  If the process
        has already exited, nothing happens."""
        self._in_queue.put(input_line)

    def get_output(self) -> str | None:
        """Returns any new output that the Odamex console has generated since the previous
        call to this function, or None if there is no output."""
        try:
            return self._out_queue.get_nowait()
        except:
            return None


class OdamexClient:
    """This class represents a launchable Odamex client executable.
    It requires the caller to specify the executable to launch. It also can accept
    a current-working-directory from which to launch the executable.  If not given,
    the executable's directory is used as cwd."""

    def __init__(self,
                 executable: str | Path,
                 cwd:        str | Path | None = None):
        if not isinstance(executable, Path):
            executable = Path(executable)

        if cwd is not None and not isinstance(cwd, Path):
            cwd = Path(cwd)

            if not cwd.is_dir():
                raise FileNotFoundError(f"No such directory to be used for CWD: {cwd}")

        if not executable.is_file():
            raise FileNotFoundError(f"No such Odamex executable: {executable}")

        self.executable = executable.resolve()
        self.cwd        = cwd.resolve() if cwd is not None else self.executable.parent

    def launch(self, *args) -> OdamexProcess:
        """Launches the Odamex executable with the given arguments and returns the
        resulting OdamexProcess instance."""

        process = subprocess.Popen([ self.executable ] + list(args),
                                   cwd    = self.cwd,
                                   text   = True,
                                   stdin  = subprocess.PIPE,
                                   stdout = subprocess.PIPE,
                                   stderr = subprocess.STDOUT,
                                   )
        return OdamexProcess(process)

class Snoopy:
    """This class snoops on the Odamex console output and sends in console inputs
    based on the content of the output."""

    def __init__(self,
                 client_process: OdamexProcess):
        self.client_process = client_process
        self.pid            = client_process.pid

    def update(self) -> int | None:
        """Performs periodic processing of the owned OdamexProcess and does the actual
        business logic of controlling the client.  Returns None if the process is still
        running, or an exit code if the process has exited."""

        result      = self.client_process.poll()
        output_line = self.client_process.get_output()

        if output_line is not None:
            sys.stdout.write(f"{self.pid}: {output_line}")
            if output_line.endswith(" now\n"):
                commander_name = output_line.split(':')[0]

                self.client_process.send_input(f'spy "{commander_name}"\n')
            elif output_line.endswith(" quit\n"):
                self.client_process.send_input(f"quit\n")

        return result

def main():
    parser = argparse.ArgumentParser(
            description="Launch a client whose console is under the control of this script.",
            add_help=False,     # We don't want the '-h' because it conflicts with '-height.'  We'll add --help explicitly.
            )
    parser.add_argument('executable', metavar='executable [ args ]', type=str, help="The odamex client executable to run, followed by any number of arguments to pass to it.")

    parser.add_argument('-j', '--jobs',      type=int, default=1, help="The number of clients to launch and manage.  Defaults to 1.")
    parser.add_argument('--no-print-stdout', action='store_true', help="Suppress output from the client.  Please note that this will prevent the script from being able to understand chat commands.")
    parser.add_argument('--cwd',             type=str,            help="The current working directory to use.  Defaults to the executable's directory.")
    parser.add_argument('--help',            action='help',       help="Display this help message.")

    args, remaining_args = parser.parse_known_args()

    client = OdamexClient(args.executable, args.cwd)

    if not args.no_print_stdout:
        remaining_args += ['+print_stdout','1']     # Without this, the script can't monitor the clients' consoles.

    snoopers = tuple(Snoopy(client.launch(*remaining_args)) for _ in range(args.jobs))

    while True:
        results = set(s.update() for s in snoopers)
        if None not in results:
            break

if __name__ == '__main__':
    main()
