## Holographic Acousto-optic System Software Overview
This repository contains information regarding the software used for the Openwater holographic acousto-optic imaging system.

## New System Bring-up
- [Control Computer Bring-up](opw_acousto-optic_NewComputerBring-up.docx)
- [Basic System Operation Manual](opw_acousto-optic_SystemOperationManual.docx)

With a completely built setup, the following commands run in an Anaconda Powershell Prompt will start the scanner UI:
```
cd ../../Openwater/scan/opw_acousto-optic_sw/system/app
conda activate owi
jupyter notebook
```

## Processing Data in opw_acousto-optic_sw Repository
Instructions:
- Open [ao_viewer.py](analysis/ao_viewer.py) script within [Spyder](https://www.spyder-ide.org/), preferably installed as a part of [Anaconda](https://www.anaconda.com/download).
- Clone the example data repository ([opw_acousto-optic_data](https://github.com/OpenwaterInternet/opw_acousto-optic_data/)).
- Update directories within the ao_viewer.py and run the file.

## Additional acousto-optic repositories here:
- [Link to the hardware repository](https://github.com/OpenwaterInternet/opw_acousto-optic_hw/)
- [Link to the example data repository](https://github.com/OpenwaterInternet/opw_acousto-optic_data/)

## License
opw_acousto-optic_sw is licensed under the GNU Affero General Public License v3.0. See [LICENSE](LICENSE) for details.

## Investigational Use Only
CAUTION - Investigational device. Limited by Federal (or United States) law to investigational use. opw_acousto-optic_sw has *not* been evaluated by the FDA and is not designed for the treatment or diagnosis of any disease. It is provided AS-IS, with no warranties. User assumes all liability and responsibility for identifying and mitigating risks associated with using this software.
