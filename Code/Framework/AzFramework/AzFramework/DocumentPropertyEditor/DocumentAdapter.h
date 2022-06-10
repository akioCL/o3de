/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ::DocumentPropertyEditor
{
    // Forward declarations
    class DocumentAdapter;
    class RoutingAdapter;

    using DocumentAdapterPtr = AZStd::shared_ptr<DocumentAdapter>;
    using ConstDocumentAdapterPtr = AZStd::shared_ptr<const DocumentAdapter>;

    struct AdapterMessage
    {
        //! The name of this message (derived from the CallbackAttribute's name)
        AZ::Name m_messageName;
        //! The path in the adapter from which this message originated
        Dom::Path m_messageOrigin;
        //! A DOM array value containing marshalled arguments for this message.
        Dom::Value m_messageParameters;
        //! An arbitrary, user-specified DOM value containing additional context data.
        //! This is used to supplement message parameters on the adapter level.
        Dom::Value m_contextData;

        Dom::Value Match() const
        {
            return {};
        }

        template <typename CallbackAttribute, typename Callback, typename... Rest>
        Dom::Value Match(const CallbackAttribute& attribute, const Callback& callback, Rest... rest) const
        {
            Dom::Value result;
            if (attribute.MatchMessage(*this, result, callback))
            {
                return result;
            }
            return Match(rest...);
        }
    };

    struct BoundAdapterMessage
    {
        DocumentAdapter* m_adapter;
        AZ::Name m_messageName;
        Dom::Path m_messageOrigin;
        Dom::Value m_contextData;

        Dom::Value operator()(const Dom::Value& parameters);

        static const AZ::Name s_typeField;
        static constexpr AZStd::string_view s_typeName = "BoundAdapterMessage";
        static const AZ::Name s_adapterField;
        static const AZ::Name s_messageNameField;
        static const AZ::Name s_messageOriginField;
        static const AZ::Name s_contextDataField;

        //! Marshal this bound message to a DOM::Value.
        //! This is not a serializable DOM::Value, as m_adapter is stored as a pointer.
        Dom::Value MarshalToDom() const;
        //! Attempts to recreate a BoundAdapterMessage from a value marshalled via MarshalToDom.
        //! Returns the BoundAdapterMessage if successful.
        static AZStd::optional<BoundAdapterMessage> TryMarshalFromDom(const Dom::Value& value);
    };

    //! A DocumentAdapter provides an interface for transforming data from an arbitrary
    //! source into a DOM hierarchy that can be viewed and edited by a DocumentPropertyView.
    //!
    //! A DocumentAdapter shall provide hierarchical contents in the form of a DOM structure in a
    //! node-based hierarchy comprised of:
    //! - A root Adapter element that contains any number of child rows. A mininmal correct adapter may be represented
    //!   as an empty Adapter element.
    //! - Row elements, that may contain any number of property editors or rows within. Nested rows will be displayed
    //!   as children within the hierarchy, while all other nodes will be turned into widget representations
    //!   and laid out horizontally within the row.
    //! - Label elements displayed as textual labels within a Row. Labels may have attributes, but no children.
    //! - PropertyEditor elements that display a property editor of an arbitrary type, specified by the mandatory
    //!   "type" attribute. The Document Property View will scan for a registered property editor of this type, and provide
    //!   this node to the property editor for rendering. The contents of a PropertyEditor are dictated by its type.
    class DocumentAdapter
    {
    public:
        AZ_RTTI(DocumentAdapter, "{8CEFE485-45C2-4ECC-B9D1-BBE75C7B02AB}");

        using ResetEvent = Event<>;
        using ChangedEvent = Event<const Dom::Patch&>;
        using MessageEvent = Event<const AdapterMessage&, Dom::Value&>;

        virtual ~DocumentAdapter() = default;

        //! Retrieves the contents of this adapter.
        //! These contents will be lazily initialized and kept cached, allowing cheap access.
        //! Adapters may send notifications via reset and change notifications to indicate when the
        //! internal state has changed.
        Dom::Value GetContents() const;

        //! Connects a listener for the reset event, fired when the contents of this adapter have completely changed.
        //! Any views listening to this adapter will need to call GetContents to retrieve the new contents of the adapter.
        void ConnectResetHandler(ResetEvent::Handler& handler);
        //! Connects a listener for the changed event, fired when the contents of the adapter have changed.
        //! The provided patch contains all the changes provided (i.e. it shall apply cleanly on top of the last
        //! GetContents() result).
        void ConnectChangedHandler(ChangedEvent::Handler& handler);
        void ConnectMessageHandler(MessageEvent::Handler& handler);

        //! Sets a router responsible for chaining nested adapters, if supported.
        //! \see RoutingAdapter
        virtual void SetRouter(RoutingAdapter* router, const Dom::Path& route);

        Dom::Value SendMessage(const AdapterMessage& message);

        //! If true, debug mode is enabled for all DocumentAdapters.
        //! \see SetDebugModeEnabled
        static bool IsDebugModeEnabled();
        //! Enables or disables debug mode globally for all DocumentAdapters.
        //! Debug mode adds expensive extra steps to notification operations to ensure the
        //! adapter is behaving correctly and will report warnings if an issue is detected.
        //! This can also be set at runtime with the `ed_debugDocumentPropertyEditorUpdates` CVar.
        static void SetDebugModeEnabled(bool enableDebugMode);

    protected:
        //! Generates the contents of this adapter. This must be an Adapter DOM node.
        //! These contents will be cached - to notify clients of changes to the structure,
        //! NotifyResetDocument or NotifyContentsChanged must be used.
        //! \see AdapterBuilder for building out this DOM structure.
        virtual Dom::Value GenerateContents() = 0;

        virtual Dom::Value HandleMessage(const AdapterMessage& message);

        //! Specifies the type of reset operation triggered in NotifyResetDocument.
        enum class DocumentResetType
        {
            //! (Default) On soft reset, the adapter will compare any existing cached contents to the new result of
            //! GetContents and produce patches based on the difference, assuming cached contents are available.
            SoftReset,
            //! On hard reset, the adapter will clear any cached contents and simply emit a reset event, ensuring
            //! any views fully reset their contents. In cases where the new adapter contents are fully disparate,
            //! this can be more efficient than the comparison from a soft reset.
            HardReset,
        };

        //! Subclasses may call this to trigger a ResetEvent and let the view know that GetContents should be requeried.
        //! Where possible, prefer to use NotifyContentsChanged instead.
        void NotifyResetDocument(DocumentResetType resetType = DocumentResetType::SoftReset);
        //! Subclasses may call this to trigger a ChangedEvent to notify the view that this adapter's contents have changed.
        //! This patch should apply cleanly on the last result GetContents would have returned after any preceding changed
        //! or reset events.
        void NotifyContentsChanged(const Dom::Patch& patch);

    private:
        ResetEvent m_resetEvent;
        ChangedEvent m_changedEvent;
        MessageEvent m_messageEvent;

        mutable Dom::Value m_cachedContents;
    };
} // namespace AZ::DocumentPropertyEditor

namespace AZ
{
    namespace Dpe = AZ::DocumentPropertyEditor;
} // namespace AZ
