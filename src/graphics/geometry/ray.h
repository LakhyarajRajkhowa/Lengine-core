//#pragma once
//
//#define GLM_ENABLE_EXPERIMENTAL 
//#include <glm/glm.hpp>
//#include <glm/gtx/norm.hpp>
//#include <iostream>
//namespace Lengine {
//    inline glm::vec3 ComputeRayDirection(
//        float mouseX,
//        float mouseY,
//        float screenWidth,
//        float screenHeight,
//        const glm::mat4& view,
//        const glm::mat4& projection)
//    {
//      
//        float x = (2.0f * mouseX) / screenWidth - 1.0f;
//        float y = 1.0f - (2.0f * mouseY) / screenHeight;
//        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
//        glm::vec4 rayEye = glm::inverse(projection) * rayClip;
//        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
//        glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
//
//        return rayDir;
//    }
//
//
//    inline bool rayIntersectsSphere(
//        const glm::vec3& rayOrigin,
//        const glm::vec3& rayDir,
//        const glm::vec3& center,
//        float radius
//    ) {
//        
//
//        glm::vec3 oc = rayOrigin - center;
//
//        float a = glm::dot(rayDir, rayDir);
//        float b = 2.0f * glm::dot(oc, rayDir);
//        float c = glm::dot(oc, oc) - radius * radius;
//
//        float discriminant = b * b - 4.0f * a * c;
//        bool hit = (discriminant >= 0.0f);
//        return hit;
//    }
//
//    inline bool rayIntersectsCapsule(
//        const glm::vec3& rayOrigin,
//        const glm::vec3& rayDir,
//        const glm::vec3& a,
//        const glm::vec3& b,
//        float radius,
//        float& outT
//    ) {
//        const float EPS = 1e-6f;
//
//        glm::vec3 ab = b - a;
//        glm::vec3 ao = rayOrigin - a;
//
//        float abLen2 = glm::dot(ab, ab);
//        float abDotDir = glm::dot(ab, rayDir);
//        float abDotAO = glm::dot(ab, ao);
//
//        float dirDotAO = glm::dot(rayDir, ao);
//
//        float denom = abLen2 - abDotDir * abDotDir;
//
//        float tRay, tSeg;
//
//        // ---------------------------------------------
//        // Closest points between ray and segment
//        // ---------------------------------------------
//        if (fabs(denom) > EPS) {
//            tRay = (abDotDir * abDotAO - abLen2 * dirDotAO) / denom;
//            tSeg = (abDotAO + tRay * abDotDir) / abLen2;
//        }
//        else {
//            // Ray and segment nearly parallel
//            tRay = -dirDotAO;
//            tSeg = abDotAO / abLen2;
//        }
//
//        // Clamp segment parameter
//        tSeg = glm::clamp(tSeg, 0.0f, 1.0f);
//        tRay = glm::max(tRay, 0.0f);
//
//        glm::vec3 closestRay = rayOrigin + tRay * rayDir;
//        glm::vec3 closestSeg = a + tSeg * ab;
//
//        float dist2 = glm::length2(closestRay - closestSeg);
//
//        if (dist2 > radius * radius)
//            return false;
//
//        outT = tRay;
//        return true;
//    }
//
//
//    inline glm::vec3 RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
//        glm::vec3 planeNormal, float planeHeight)
//    {
//        float denom = glm::dot(rayDir, planeNormal);
//        if (fabs(denom) < 0.0001f) return rayOrigin; // parallel
//
//        float t = (planeHeight - glm::dot(rayOrigin, planeNormal)) / denom;
//        return rayOrigin + rayDir * t;
//    }
//
//    inline glm::vec3 computeAxisPlaneHit(
//        const glm::vec3& rayOrigin,
//        const glm::vec3& rayDir,
//        const glm::vec3& axisDir,
//        const glm::vec3& pivot,
//        const glm::vec3& camPos
//    ) {
//        glm::vec3 camDir = glm::normalize(camPos - pivot);
//
//        glm::vec3 planeNormal =
//            glm::normalize(glm::cross(axisDir, glm::cross(camDir, axisDir)));
//
//        float d = glm::dot(planeNormal, pivot);
//        float t = (d - glm::dot(planeNormal, rayOrigin)) /
//            glm::dot(planeNormal, rayDir);
//
//        return rayOrigin + rayDir * t;
//    }
//
//
//
//}
//
