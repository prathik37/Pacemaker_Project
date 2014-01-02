//This file was generated from (Academic) UPPAAL 4.0.13 (rev. 4577), September 2010

/*

*/
A[] (Pacemaker_Process.VSenseIgnoredS1a imply Pacemaker_Process.v_clk <= Pacemaker_Process.VRP)

/*

*/
A[] !(Pacemaker_Process.a_clk < Pacemaker_Process.AVI &&  Monitor_Process.VPaced)

/*

*/
A[] (Pacemaker_Process.ASenseIgnoredS1a imply Pacemaker_Process.v_clk <= Pacemaker_Process.PVARP) && (Pacemaker_Process.ASenseIgnoredS2 imply Pacemaker_Process.v_clk <= Pacemaker_Process.PVARP)

/*

*/
A[] (Monitor_Process.VPaced imply Monitor_Process.vmClock <= Monitor_Process.mLRI)

/*

*/
A[] !(Monitor_Process.ASensed  && Monitor_Process.VSensed)

/*

*/
A[] !(Monitor_Process.APaced && Monitor_Process.VPaced)

/*

*/
A[] not deadlock\

