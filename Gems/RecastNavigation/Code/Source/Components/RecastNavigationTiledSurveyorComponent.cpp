/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationTiledSurveyorComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <DebugDraw/DebugDrawBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

AZ_CVAR(
    bool, cl_navmesh_showInputData, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draws triangle mesh input data that was used for the navigation mesh calculation");
AZ_CVAR(
    float, cl_navmesh_showInputDataSeconds, 30.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, keeps the debug triangle mesh input for the specified number of seconds");

namespace RecastNavigation
{
    void RecastNavigationTiledSurveyorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationTiledSurveyorComponent, AZ::Component>()
                ->Version(1)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RecastNavigationTiledSurveyorComponent>()->RequestBus("RecastNavigationSurveyorRequestBus");
        }
    }

    void RecastNavigationTiledSurveyorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationTiledSurveyorComponent"));
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void RecastNavigationTiledSurveyorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationTiledSurveyorComponent"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void RecastNavigationTiledSurveyorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("BoxShapeService"));
    }

    void RecastNavigationTiledSurveyorComponent::CollectGeometryWithinVolume(const AZ::Aabb& volume, ShapeHits& overlapHits)
    {
        AZ::Vector3 dimension = volume.GetExtents();
        AZ::Transform pose = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), volume.GetCenter());

        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimension;

        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(dimension, pose, nullptr);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        request.m_collisionGroup = AzPhysics::CollisionGroup::All;

        AzPhysics::SceneQuery::UnboundedOverlapHitCallback unboundedOverlapHitCallback =
            [&overlapHits](AZStd::optional<AzPhysics::SceneQueryHit>&& hit)
        {
            if (hit && ((hit->m_resultFlags & AzPhysics::SceneQuery::EntityId) != 0))
            {
                const AzPhysics::SceneQueryHit& sceneQueryHit = *hit;
                overlapHits.emplace_back(sceneQueryHit.m_entityId, sceneQueryHit.m_shape);
            }

            return true;
        };

        request.m_unboundedOverlapHitCallback = unboundedOverlapHitCallback;

        auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);

        // Note: blocking call
        AZ::Interface<AzPhysics::SceneInterface>::Get()->QueryScene(sceneHandle, &request);
        // results are in overlapHits
    }

    void RecastNavigationTiledSurveyorComponent::AppendColliderGeometry(
        TileGeometry& geometry,
        const ShapeHits& overlapHits)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;
        AZStd::size_t indicesCount = geometry.m_indices.size();

        for (const AZStd::pair<AZ::EntityId, Physics::Shape*>& overlapHit : overlapHits)
        {
            AZ::EntityId hitEntityId = overlapHit.first;

            AZ::Transform t = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(t, hitEntityId, &AZ::TransformBus::Events::GetWorldTM);
            // Physical geometry already takes scale into account. Discard the scale.
            t.SetUniformScale(1.0f);

            overlapHit.second->GetGeometry(vertices, indices, nullptr);

            if (!vertices.empty())
            {
                if (indices.empty())
                {
                    AZStd::vector<AZ::Vector3> transformed;

                    int currentLocalIndex = 0;
                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        geometry.m_vertices.push_back(RecastVector3(translated));

                        if (cl_navmesh_showInputData)
                        {
                            transformed.push_back(translated);
                        }

                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + currentLocalIndex));
                        currentLocalIndex++;
                    }

                    if (cl_navmesh_showInputData)
                    {
                        for (size_t i = 2; i < vertices.size(); i += 3)
                        {
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[i - 2], transformed[i - 1], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[i - 1], transformed[i - 0], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[i - 0], transformed[i - 2], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                        }
                    }
                }
                else
                {
                    AZStd::vector<AZ::Vector3> transformed;

                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        geometry.m_vertices.push_back(RecastVector3(translated));

                        if (cl_navmesh_showInputData)
                        {
                            transformed.push_back(translated);
                        }
                    }

                    for (size_t i = 2; i < indices.size(); i += 3)
                    {
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 1]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 2]));
                    }

                    if (cl_navmesh_showInputData)
                    {
                        for (size_t i = 2; i < indices.size(); i += 3)
                        {
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[indices[i - 2]], transformed[indices[i - 1]], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[indices[i - 1]], transformed[indices[i - 0]], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[indices[i - 0]], transformed[indices[i - 2]], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                        }
                    }
                }

                indicesCount += vertices.size();
                vertices.clear();
                indices.clear();
            }
        }
    }

    void RecastNavigationTiledSurveyorComponent::Activate()
    {
        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationSurveyorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RecastNavigationTiledSurveyorComponent::Deactivate()
    {
        RecastNavigationSurveyorRequestBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationTiledSurveyorComponent::CollectGeometry(
        float tileSize, float borderSize)
    {
        if (tileSize == 0.f)
        {
            return {};
        }

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        const AZ::Vector3& worldMin = worldVolume.GetMin();
        const AZ::Vector3& worldMax = worldVolume.GetMax();

        const AZ::Vector3 border = AZ::Vector3::CreateOne() * borderSize;

        for (int y = 0; y < tilesAlongY; ++y)
        {
            for (int x = 0; x < tilesAlongX; ++x)
            {
                const AZ::Vector3 tileMin{
                    worldMin.GetX() + aznumeric_cast<float>(x) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y) * tileSize,
                    worldMin.GetZ()
                };

                const AZ::Vector3 tileMax{
                    worldMin.GetX() + aznumeric_cast<float>(x + 1) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y + 1) * tileSize,
                    worldMax.GetZ()
                };

                AZ::Aabb tileVolume = AZ::Aabb::CreateFromMinMax(tileMin, tileMax);
                AZ::Aabb scanVolume = AZ::Aabb::CreateFromMinMax(tileMin - border, tileMax + border);

                ShapeHits results;
                AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();
                geometryData->m_worldBounds = tileVolume;
                CollectGeometryWithinVolume(scanVolume, results);

                AppendColliderGeometry(*geometryData, results);

                geometryData->m_tileX = x;
                geometryData->m_tileY = y;
                tiles.push_back(geometryData);
            }
        }

        return tiles;
    }

    AZ::Aabb RecastNavigationTiledSurveyorComponent::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }

    int RecastNavigationTiledSurveyorComponent::GetNumberOfTiles(float tileSize) const
    {
        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        const int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        const int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        return tilesAlongX * tilesAlongY;
    }
} // namespace RecastNavigation
