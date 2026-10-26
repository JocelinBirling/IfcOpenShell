// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_COMPONENTS_FILEDIALOGPATHS_H
#define IFCINTERFACE_COMPONENTS_FILEDIALOGPATHS_H

class QFileDialog;

namespace bonsaiviewer::components {

// BonsaiViewer22 (Easy Load File): the "Look in" bar of a non-native QFileDialog
// accepts a pasted path. Enter with an existing folder path opens that folder;
// Enter with an existing file path opens the folder that contains the file.
// Any other text is left to the dialog. A native dialog has no such bar: nothing
// happens there.
void acceptPastedPaths(QFileDialog& dialog);

} // namespace bonsaiviewer::components

#endif // IFCINTERFACE_COMPONENTS_FILEDIALOGPATHS_H
