# WinPrintServer
Windows TCP Print Server that will forward raw data directly to a printer

## Usage
Run `WinPrintServer.exe` without any commandline arguments to present your default printer as a RAW printer on port 9100.

To present a printer other than the default printer, you can specify the printer name on the commandline.

```
WinPrintServer [options] [printername]
  -h           Show this help information
  printername  The name of the printer to print. If not specified the default printer is used
```

## Example

Present a printer called `EPSON098CEF (WF-3520 Series)` as a RAW printer use the following command. 

```
WinPrintServer "EPSON098CEF (WF-3520 Series)"
```

*Note* - Since the printer name has spaces, the name is quoted on the command line