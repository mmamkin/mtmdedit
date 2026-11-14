// Self-extracting installer stub for MtmdEdit.
// make-setup.ps1 compiles it with the csc.exe that ships with Windows (.NET
// Framework 4.x) and embeds the payload files as resources. This replaced an
// IExpress package: IExpress fails silently on current Windows 11 builds.
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;

static class SetupStub
{
    const string PayloadPrefix = "payload/";

    static int Main(string[] args)
    {
        bool quiet = args.Any(a => a.Equals("/q", StringComparison.OrdinalIgnoreCase)
                                || a.Equals("/quiet", StringComparison.OrdinalIgnoreCase));
        string dir = Path.Combine(Path.GetTempPath(), "MtmdEditSetup-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(dir);
        try
        {
            Assembly asm = Assembly.GetExecutingAssembly();
            foreach (string name in asm.GetManifestResourceNames().Where(n => n.StartsWith(PayloadPrefix)))
            {
                using (Stream src = asm.GetManifestResourceStream(name))
                using (FileStream dst = File.Create(Path.Combine(dir, name.Substring(PayloadPrefix.Length))))
                    src.CopyTo(dst);
            }
            var psi = new ProcessStartInfo("powershell.exe",
                "-NoProfile -ExecutionPolicy Bypass -File \"" + Path.Combine(dir, "setup.ps1") + "\"" + (quiet ? " -Quiet" : ""))
            {
                UseShellExecute = false,
                // The script shows its own dialogs (none in quiet mode); no console.
                CreateNoWindow = true,
            };
            using (Process p = Process.Start(psi))
            {
                p.WaitForExit();
                return p.ExitCode;
            }
        }
        finally
        {
            try { Directory.Delete(dir, true); } catch { /* temp folder, best effort */ }
        }
    }
}
