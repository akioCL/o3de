/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabDependencyViewer/PrefabDependencyViewer.h>
#include <QLabel>
#include <QBoxLayout>

namespace AzToolsFramework
{
    PrefabDependencyViewerWidget::PrefabDependencyViewerWidget(QWidget* pParent, Qt::WindowFlags flags)
        : QWidget(pParent, flags)
    {
        displayText();
    }

    void PrefabDependencyViewerWidget::displayText()
    {
        
        setStyleSheet("QWidget{ background-color : rgba( 160, 160, 160, 255); border-radius : 7px;  }");
        QLabel* label = new QLabel(this);
        QHBoxLayout* layout = new QHBoxLayout();
        label->setText(AZStd::string("Random String").c_str());
        layout->addWidget(label);
        setLayout(layout);
    }

    /* void PrefabDependencyViewerWidget::displayTree(AzToolsFramework::Prefab::Instance& prefab)
    {
        QLabel* label = new QLabel(this);
        QHBoxLayout* layout = new QHBoxLayout();

        label->setText(prefab.GetAbsoluteInstanceAliasPath().c_str());
        layout->addWidget(label);
        setLayout(layout);
    }*/
}
