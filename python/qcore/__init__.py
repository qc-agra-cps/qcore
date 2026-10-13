from pyqcore import (
    FileFormat, RotationType, ConditionType, GateType, AssertionInfo,
    ClassicalCondition, QGate, QCircuit, DecompositionConfiguration,

    decomposition
)
import sys

decomposition.configuration = DecompositionConfiguration

def circuit_to_qiskit(circuit: QCircuit):
    """
      Requires the packages qiskit,qiksit_qasm3_import
    """
    from qiskit.qasm3 import loads
    circuit = decomposition.resolve_meta_gates(circuit)
    qasm3 = circuit.toString(FileFormat.OpenQASM3)
    return loads(qasm3)

def draw_circuit(circuit: QCircuit, maxwidth=80):
    def run(_gates, first):
        gates = _gates
        q_id = {}
        lines=[]
        for i, q in enumerate(circuit.getQubits()):
            q_id[q] = i;
            lines += [('' if first else  '<... ') + f'q_{q}:']


        maxlinelen = max([len(l) for l in lines])
        lines = [l + ' ' * (maxlinelen-len(l)+1)  for l in lines]
        linelen = maxlinelen+1

        last = gates
        last_lines = lines
        while len(gates) > 0:
            gate = gates[0]
            gates = gates[1:]

            name = str(gate.getType()).split('.')[-1]
            docolor = True
            if name == 'CX':
                docolor = False
                name  = 'X'
            if name == 'CZ':
                docolor = False
                name  = '●'
            if name == "BARRIER":
                for i in range(len(lines)):
                    lines[i] += '┼'
                continue

            ctrl = [q_id[q] for q in gate.getControls()]
            targ = [q_id[q] for q in gate.getTargets()]
            q_from = min(ctrl+targ)
            q_to = max(ctrl+targ)

            for i in range(len(lines)):
                if i < q_from or i > q_to:
                    lines[i] += '─' * len(name)
                    continue
                if i in ctrl:
                    pre = len(name)//2
                    lines[i] += '─' * pre + '●' + '─' * (len(name)-(pre+1))
                    continue
                if i in targ:
                    if 'ipykernel' in sys.modules:
                        RESET = '</span>'
                        INVERT = (
                            '<span style="color: black; background-color: #b4b4b4;">'
                            if len(gates)%2 == 1 else
                            '<span style="color: black; background-color: #d4d4d4;">'
                        )
                    else:
                        RESET = '\033[0m'
                        INVERT = '\033[7;38;5;252m' if len(gates)%2 == 1 else '\033[7;38;5;245m'
                    lines[i] += INVERT + name + RESET if docolor else name
                    continue
                pre = len(name)//2
                lines[i] += '─' * pre + '┼' + '─' * (len(name)-(pre+1))
            linelen += len(name)

            if linelen > maxwidth:
                gates = last
                lines = last_lines
                break

            last = gates
            last_lines = lines
        assert len(gates) == 0 or len(gates) != len(_gates)
        return gates, lines

    gates = circuit.getGates()
    first = True
    while len(gates) > 0:
        gates, lines = run(gates, first)
        first = False
        if 'ipykernel' in sys.modules:
            from IPython.display import HTML, display
            display(HTML(
                '<pre><span style="font-family: monospace; line-height: 1em;">'
                + '<br/>'.join(lines)
                + '<br/></span><br/></pre>'
            ))
        else:
            print('\n'.join(lines) + '\n')
