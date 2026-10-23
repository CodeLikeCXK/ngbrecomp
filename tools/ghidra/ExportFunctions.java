// After analysis, to the file named by the script's argument: "F <entry>" for every function, and
// "J <source> <target>" / "C <source> <target>" for every direct jump and call (hex).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import java.io.PrintWriter;

public class ExportFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        int n = 0;
        try (PrintWriter w = new PrintWriter(getScriptArgs()[0])) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(true))
                w.printf("F %X%n", f.getEntryPoint().getOffset());
            for (Instruction i : currentProgram.getListing().getInstructions(true)) {
                boolean jump = i.getFlowType().isJump(), call = i.getFlowType().isCall();
                if (!jump && !call) continue;
                for (Address t : i.getFlows()) {
                    w.printf("%s %X %X%n", jump ? "J" : "C", i.getAddress().getOffset(), t.getOffset());
                    n++;
                }
            }
        }
        println("exported " + n + " flows");
    }
}
