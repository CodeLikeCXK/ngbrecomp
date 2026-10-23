// Before analysis: disassemble and make a function at each address in the file named by the script's
// argument (hex, one a line) -- the starts the recompiler's own analysis found. tools/ghidra_functions.sh
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import java.nio.file.*;

public class SeedStarts extends GhidraScript {
    @Override
    public void run() throws Exception {
        int n = 0;
        for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
            line = line.trim();
            if (line.isEmpty()) continue;
            Address a = toAddr(Long.parseLong(line, 16));
            disassemble(a);
            if (getFunctionAt(a) == null && createFunction(a, null) != null) n++;
        }
        println("seeded " + n + " functions");
    }
}
