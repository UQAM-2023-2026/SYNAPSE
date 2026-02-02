"""
ChopExecute script to launch SYNAPSE UI
Place this in a ChopExecute DAT in TouchDesigner
Stores the process reference in parent().stored() for later termination
"""

import subprocess
import os

def onOffToOn(channel, sampleIndex, val, prev):
	"""
	Triggered when channel goes from 0 to 1
	"""
	# Get the path relative to the project file
	project_folder = project.folder
	command_path = os.path.join(project_folder, '../UI/LAUNCH_SYNAPSE.command')
	command_path = os.path.abspath(command_path)

	# Check if file exists
	if os.path.exists(command_path):
		try:
			# Launch the command file and store the process reference
			process = subprocess.Popen(['open', command_path])

			# Store process in parent for access by stop script
			parent().store('synapse_process', process)

			print(f"Launched: {command_path}")
			print(f"Process ID: {process.pid}")
		except Exception as e:
			print(f"Error launching command: {e}")
	else:
		print(f"Command file not found: {command_path}")

	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	return

def onPulse(channel, sampleIndex, val, prev):
	return
