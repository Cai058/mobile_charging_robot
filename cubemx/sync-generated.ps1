[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet('can', 'gpio', 'usart1_rc', 'tim2', 'rfid_uart8', 'battery_usart6', 'server_uart7')]
    [string]$Peripheral = 'can'
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$mappings = @{
    can = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/can.c'
            Destination = Join-Path $repoRoot 'User/Src/can.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/can.h'
            Destination = Join-Path $repoRoot 'User/Inc/can.h'
            NormalizeTrailingBlankLines = $true
        }
    )
    gpio = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/gpio.c'
            Destination = Join-Path $repoRoot 'User/Src/gpio.c'
        }
    )
    usart1_rc = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/usart.c'
            Destination = Join-Path $repoRoot 'User/Src/usart.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/usart.h'
            Destination = Join-Path $repoRoot 'User/Inc/usart.h'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/dma.c'
            Destination = Join-Path $repoRoot 'User/Src/dma.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/dma.h'
            Destination = Join-Path $repoRoot 'User/Inc/dma.h'
        }
    )
    tim2 = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/tim.c'
            Destination = Join-Path $repoRoot 'User/Src/tim.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/tim.h'
            Destination = Join-Path $repoRoot 'User/Inc/tim.h'
        }
    )
    rfid_uart8 = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/usart.c'
            Destination = Join-Path $repoRoot 'User/Src/usart.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/usart.h'
            Destination = Join-Path $repoRoot 'User/Inc/usart.h'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/dma.c'
            Destination = Join-Path $repoRoot 'User/Src/dma.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/dma.h'
            Destination = Join-Path $repoRoot 'User/Inc/dma.h'
        }
    )
    battery_usart6 = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/usart.c'
            Destination = Join-Path $repoRoot 'User/Src/usart.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/usart.h'
            Destination = Join-Path $repoRoot 'User/Inc/usart.h'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/dma.c'
            Destination = Join-Path $repoRoot 'User/Src/dma.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/dma.h'
            Destination = Join-Path $repoRoot 'User/Inc/dma.h'
        }
    )
    server_uart7 = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/usart.c'
            Destination = Join-Path $repoRoot 'User/Src/usart.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/usart.h'
            Destination = Join-Path $repoRoot 'User/Inc/usart.h'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/dma.c'
            Destination = Join-Path $repoRoot 'User/Src/dma.c'
        },
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Inc/dma.h'
            Destination = Join-Path $repoRoot 'User/Inc/dma.h'
        }
    )
}

foreach ($mapping in $mappings[$Peripheral]) {
    $source = $mapping.Source
    $destination = $mapping.Destination

    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "CubeMX generated file not found: $source. Run Generate Code first."
    }

    if ($mapping.NormalizeTrailingBlankLines) {
        $sourceText = [IO.File]::ReadAllText($source)
        $normalizedSourceText = [regex]::Replace($sourceText, '(\r?\n)+\z', "`r`n")
        if ($sourceText -ne $normalizedSourceText) {
            [IO.File]::WriteAllText(
                $source,
                $normalizedSourceText,
                [Text.UTF8Encoding]::new($false)
            )
            Write-Host "Normalized trailing blank lines: $source"
        }
    }

    $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    $destinationHash = if (Test-Path -LiteralPath $destination -PathType Leaf) {
        (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    } else {
        $null
    }

    if ($sourceHash -eq $destinationHash) {
        Write-Host "Already synchronized: $destination"
        continue
    }

    if ($PSCmdlet.ShouldProcess($destination, "Copy CubeMX generated file from $source")) {
        Copy-Item -LiteralPath $source -Destination $destination -Force

        $copiedHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
        if ($copiedHash -ne $sourceHash) {
            throw "SHA-256 verification failed after copy: $destination"
        }

        Write-Host "Synchronized: $source -> $destination"
        Write-Host "SHA-256: $sourceHash"
    }
}
