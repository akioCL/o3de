/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SceneSettingsCard.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QPushButton>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <AzToolsFramework/Debug/TraceContextStack.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>

SceneSettingsCardHeader::SceneSettingsCardHeader(QWidget* parent /* = nullptr */)
    : AzQtComponents::CardHeader(parent)
{
    m_busyLabel = new AzQtComponents::StyledBusyLabel(this);
    m_busyLabel->SetIsBusy(true);
    m_busyLabel->SetBusyIconSize(14);
    m_backgroundLayout->insertWidget(1, m_busyLabel);

    m_closeButton = new QPushButton(this);
    m_closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_closeButton->setMinimumSize(24, 24);
    m_closeButton->setMaximumSize(24, 24);
    m_closeButton->setBaseSize(24, 24);

    m_backgroundLayout->addWidget(m_closeButton);
    
    connect(m_closeButton, &QPushButton::clicked, this, &SceneSettingsCardHeader::triggerCloseButton);
}

void SceneSettingsCardHeader::triggerCloseButton()
{
    parent()->deleteLater();
}

void SceneSettingsCardHeader::SetCanClose(bool canClose)
{
    m_closeButton->setEnabled(canClose);
    // If this card can be closed, it's not busy
    m_busyLabel->setHidden(canClose);
}

SceneSettingsCard::SceneSettingsCard(AZ::Uuid traceTag, QWidget* parent /* = nullptr */)
    :
    AzQtComponents::Card(new SceneSettingsCardHeader(), parent),
    m_traceTag(traceTag)
{
    m_settingsHeader = qobject_cast<SceneSettingsCardHeader*>(header());
    // This has to be set here, instead of in the customheader,
    // because the Card constructor forces the context menu to be visible.
    header()->setHasContextMenu(false);

    m_reportModel = new AzQtComponents::StyledDetailsTableModel();
    m_reportModel->AddColumn("Status", AzQtComponents::StyledDetailsTableModel::StatusIcon);
    m_reportModel->AddColumn("Time");
    m_reportModel->AddColumn("Message");
    m_reportModel->AddColumnAlias("message", "Message");
    AzQtComponents::StyledDetailsTableView* m_reportView = new AzQtComponents::StyledDetailsTableView();
    m_reportView->setModel(m_reportModel);
    setContentWidget(m_reportView);

    AZ::Debug::TraceMessageBus::Handler::BusConnect();
}

SceneSettingsCard::~SceneSettingsCard()
{
    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
}

void SceneSettingsCard::SetAndStartProcessingHandler(const AZStd::shared_ptr<AZ::SceneAPI::SceneUI::ProcessingHandler>& handler)
{
    AZ_Assert(handler, "Processing handler was null");
    AZ_Assert(!m_targetHandler, "A handler has already been assigned. Only one can be active per layer at any given time.");
    if (m_targetHandler)
    {
        return;
    }

    m_targetHandler = handler;

    connect(m_targetHandler.get(), &AZ::SceneAPI::SceneUI::ProcessingHandler::StatusMessageUpdated, this, &SceneSettingsCard::OnSetStatusMessage);
    //connect(m_targetHandler.get(), &AZ::SceneAPI::SceneUI::ProcessingHandler::AddLogEntry, this, &SceneSettingsCard::AddLogEntry);
    connect(m_targetHandler.get(), &AZ::SceneAPI::SceneUI::ProcessingHandler::ProcessingComplete, this, &SceneSettingsCard::OnProcessingComplete);

    handler->BeginProcessing();
}

/* void SceneSettingsCard::AddLogEntry(const AzToolsFramework::Logging::LogEntry& logEntry)
{
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", logEntry.);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
    m_reportModel->AddEntry(entry);
}*/

void SceneSettingsCard::OnProcessingComplete()
{
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", "OnProcessingComplete");
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    entry.Add("Time", GetTimeAsString());
    m_reportModel->AddEntry(entry);
    
    SetState(SceneSettingsCard::SceneSettingsCardState::Done);
    /* m_isProcessingComplete = true;
    SetUIToCompleteState();

    if (!m_encounteredIssues && m_autoCloseOnSuccess)
    {
        close();
    }
    else if (m_progressLabel != nullptr)
    {
        m_progressLabel->setText("Close the processing report to continue editing settings.");
    }*/
}

void SceneSettingsCard::OnSetStatusMessage(const AZStd::string& message)
{
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message.c_str());
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    entry.Add("Time", GetTimeAsString());
    m_reportModel->AddEntry(entry);
}


bool SceneSettingsCard::OnPrintf(const char* window, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    if (AzFramework::StringFunc::Find(window, "Success") != AZStd::string::npos)
    {
        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    }
    else if (AzFramework::StringFunc::Find(window, "Warning") != AZStd::string::npos)
    {
        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
        UpdateCompletionState(CompletionState::Warning);
    }
    else if (AzFramework::StringFunc::Find(window, "Error") != AZStd::string::npos)
    {
        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
        UpdateCompletionState(CompletionState::Error);
    }
    else
    {
        // To reduce noise in the report widget, only show success, warning and error messages.
        return false;
    }
    entry.Add("Message", message);
    entry.Add("Time", GetTimeAsString());
    //CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    return false;
}

bool SceneSettingsCard::OnError(const char* window, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    (void*)window;
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
    entry.Add("Time", GetTimeAsString());
    //CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    
    UpdateCompletionState(CompletionState::Error);
    return false;
}

bool SceneSettingsCard::OnWarning(const char* window, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    (void*)window;
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
    entry.Add("Time", GetTimeAsString());
    //CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    
    UpdateCompletionState(CompletionState::Warning);
    return false;
}

bool SceneSettingsCard::OnAssert(const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
    entry.Add("Time", GetTimeAsString());
    //CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    UpdateCompletionState(CompletionState::Error);
    return false;
}

void SceneSettingsCard::SetState(SceneSettingsCardState newState)
{
    switch (newState)
    {
    case SceneSettingsCardState::Loading:
        setTitle(tr("Loading scene settings"));
        m_settingsHeader->SetCanClose(false);
        break;
    case SceneSettingsCardState::Processing:
        setTitle(tr("Saving scene settings, and reprocessing scene file"));
        m_settingsHeader->SetCanClose(false);
        break;
    case SceneSettingsCardState::Done:
        setTitle(tr("Processing completed at %1").arg(GetTimeAsString()));
        m_settingsHeader->SetCanClose(true);
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        break;
    }
}

bool SceneSettingsCard::ShouldProcessMessage()
{
    AZStd::shared_ptr<const AzToolsFramework::Debug::TraceContextStack> stack = m_traceStackHandler.GetCurrentStack();
    if (stack)
    {
        for (size_t i = 0; i < stack->GetStackCount(); ++i)
        {
            if (stack->GetType(i) == AzToolsFramework::Debug::TraceContextStackInterface::ContentType::UuidType)
            {
                if (stack->GetUuidValue(i) == m_traceTag)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void SceneSettingsCard::UpdateCompletionState(CompletionState newState)
{
    // Use the highest encountered state
    m_completionState = AZStd::max(m_completionState, newState);
}


QString SceneSettingsCard::GetTimeAsString()
{
    return QDateTime::currentDateTime().toString(tr("hh:mm:ss ap"));
}