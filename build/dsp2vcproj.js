//
// Convert DSP projects to VS2005's project files, see here:
// http://msdn2.microsoft.com/en-us/library/e6dsak0e(VS.80).aspx
//
// Usage: CScript dsp2vcproj.js <dspfile> <vcprojfile>
//

var vcproj = new ActiveXObject("VisualStudio.VCProjectEngine.8.0");
var prj = vcproj.LoadProject(WScript.Arguments.Item(0));
prj.ProjectFile = WScript.Arguments.Item(1);
prj.Save();
